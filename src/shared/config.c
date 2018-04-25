/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* config.c - read config files and create config object                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#include "shared.h"


CCTXT *ctxt_top = NULL;
CCTXT *context  = NULL;

CSECT config_sections[CONF_SECT_MAX];

#define CFG_VV_FLAGS	(VV_AUTO_VAL|VV_LOWER_ATT|VV_REMOVE_UDRSCR)

/*
 * Matches arg substitutions of the form:
 * %N% where N <= argc
 */
void config_args_substitute( char *str, int *len )
{
	int i, l;

	l = *len;

	if( !context->argc || l < 3 )
		return;

	// 1-indexed
	if( str[0] == '%' && str[l - 1] == '%'
	 && ( i = atoi( str + 1 ) ) > 0
	 && i <= context->argc )
	{
		// it can't be too long, or we could never
		// have read the include line in the first
		// place.
		i--;
		*len = context->argl[i];
		memcpy( str, context->argv[i], *len );
		str[*len] = '\0';

		debug( "Config substitution of arg %d: %s", i + 1, str );
	}
}




// read a file until we find a config line
char __cfg_read_line[CONF_LINE_MAX];
int config_get_line( FILE *f, AVP *av )
{
	char *n, *e;

	while( 1 )
	{
		// end things on a problem
		if( !f || feof( f ) || ferror( f ) )
			break;
		if( !fgets( __cfg_read_line, CONF_LINE_MAX, f ) )
			break;

		// where are we?
		context->lineno++;

		// find the line end
		if( !( n = memchr( __cfg_read_line, '\n', 1023 ) ) )
			continue;
		*n = '\0';

		// spot sections
		if( __cfg_read_line[0] == '[' )
		{
			if( !( e = memchr( __cfg_read_line, ']', n - __cfg_read_line ) ) )
			{
				warn( "Bad section heading in file '%s': %s",
					context->source, __cfg_read_line );
				return -1;
			}
			*e = '\0';

			if( ( e - __cfg_read_line ) > 511 )
			{
				warn( "Over-long section heading in file '%s': %s",
					context->source, __cfg_read_line );
				return -1;
			}

			// grab that context
			config_choose_section( context, __cfg_read_line + 1 );

			// spotted a section
			return 1;
		}

		// returns 0 if it can cope
		if( var_val( __cfg_read_line, n - __cfg_read_line, av, CFG_VV_FLAGS ) )
			continue;

		// return any useful data
		if( av->status == VV_LINE_ATTVAL )
		{
			// but do args substitution first
			config_args_substitute( av->val, &(av->vlen) );

			return 0;
		}
	}

	return -1;
}






// step over one section
int config_ignore_section( FILE *fh )
{
	int rv;
	AVP a;

	while( ( rv = config_get_line( fh, &a ) ) == 0 )
		continue;

	return rv;
}


int config_bool( AVP *av )
{
	if( valIs( "true" ) || valIs( "yes" ) || valIs( "y" ) || ( atoi( av->val ) != 0 ) )
		return 1;

	return 0;
}



CCTXT *config_make_context( char *path, WORDS *w )
{
	CCTXT *ctx, *parent = context;
	char *str;
	int i;

	ctx = (CCTXT *) allocz( sizeof( CCTXT ) );
	snprintf( ctx->source, 4096, "%s", path );

	// copy in our arguments
	if( w && w->wc > 1 )
	{
		// when we get a w, the first arg is the path
		ctx->argc = w->wc - 1;
		ctx->argl = (int *)   allocz( ctx->argc * sizeof( int ) );
		ctx->argv = (char **) allocz( ctx->argc * sizeof( char * ) );
		for( i = 1; i < w->wc; i++ )
		{
			str = str_dup( w->wd[i], w->len[i] );
			ctx->argv[i-1] = str;
			ctx->argl[i-1] = w->len[i];
		}
	}

	if( parent )
	{
		ctx->parent = parent;

		if( parent->children )
			parent->children->next = ctx;
		parent->children = ctx;

		// inherit the section we were in
		ctx->section = parent->section;

		// and if we don't have any of our own, inherit args
		if( !ctx->argc && parent->argc )
		{
			ctx->argc = parent->argc;
			ctx->argl = parent->argl;
			ctx->argv = parent->argv;
		}
	}
	else
		config_choose_section( ctx, "main" );

	// set the global on the first call
	if( !ctxt_top )
		ctxt_top = ctx;

	return ctx;
}



int config_source_dupe( char *path )
{
	CCTXT *c = context;

	while( c )
	{
		if( !strcmp( path, c->source ) )
			return 1;

		c = c->parent;
	}

	return 0;
}



#define config_set_limit( _p, _r, _v )	\
(_p)->limits[_r] = (int64_t) _v;\
(_p)->setlim[_r] = 0x01


int config_line( AVP *av )
{
	char *d;
	int res;

	if( ( d = strchr( av->att, '.' ) ) )
	{
		d++;

		if( !strncasecmp( av->att, "limits.", 7 ) )
		{
			if( !strcasecmp( d, "procs" ) )
				res = RLIMIT_NPROC;
			else if( !strcasecmp( d, "files" ) )
				res = RLIMIT_NOFILE;
			else
				return -1;

			config_set_limit( _proc, res, atoll( av->val ) );
		}
		else
			return -1;

		return 0;
	}

	if( attIs( "tickMsec" ) )
	{
		av_int( _proc->tick_usec );
		_proc->tick_usec *= 1000;
	}
	else if( attIs( "daemon" ) )
	{
		if( config_bool( av ) )
			runf_add( RUN_DAEMON );
		else
			runf_rmv( RUN_DAEMON );
	}
	else if( attIs( "pidFile" ) )
	{
		snprintf( _proc->pidfile, CONF_LINE_MAX, "%s", av->val );
	}
	else if( attIs( "baseDir" ) )
	{
		snprintf( _proc->basedir, CONF_LINE_MAX, "%s", av->val );
	}
	return 0;
}


// always returns a new string
char *config_relative_path( char *inpath )
{
	char *ret;
	int len;

	// is it a URI?
	if( strstr( inpath, "://" ) )
		return strdup( inpath );

	if( *inpath != '~' )
		return strdup( inpath );

	if( !_proc->basedir )
		// just step over it
		return strdup( inpath + 1 );

	// add 1 for the /, remove 1 for the ~, add 1 for the \0
	len = strlen( _proc->basedir ) + strlen( inpath ) + 1;

	ret = (char *) malloc( len );
	snprintf( ret, len, "%s/%s", _proc->basedir, inpath + 1 );

	return ret;
}


int __config_read_file( FILE *fh )
{
	int rv, lrv, ret = 0;
	WORDS w;
	AVP av;

	while( ( rv = config_get_line( fh, &av ) ) != -1 )
	{
		// was it just a section change?
		if( rv > 0 )
			continue;

		// spot includes - they inherit context
		if( !strcasecmp( av.att, "include" ) )
		{
			strwords( &w, av.val, av.vlen, ' ' );

			if( config_read( w.wd[0], &w ) != 0 )
			{
				err( "Included config file '%s' is not valid.", av.val );
				ret = -1;
				break;
			}

			continue;
		}

		// and dispatch it
		lrv = (*(context->section->fp))( &av );
		ret += lrv;

		if( lrv )
		{
			err( "Bad config in file '%s', line %d", context->source, context->lineno );
			info( "Bad line: %s = %s", av.att, av.val );
			break;
		}
	}

	if( fh )
		fclose( fh );

	debug( "Finished with config source '%s': %d", context->source, ret );

	// step our context back
	context = context->parent;

	return ret;
}





int config_read_file( char *path )
{
	FILE *fh = NULL;

	debug( "Opening config file %s, section %s",
		path, context->section->name );

	// die on not reading main config file, warn on others
	if( !( fh = fopen( path, "r" ) ) )
	{
		err( "Could not open config file '%s' -- %s", path, Err );
		return -1;
	}

	return __config_read_file( fh );
}




#define CErr		curl_easy_strerror( cc )


int config_read_url( char *url )
{
	int ret = 0;
	CURLcode cc;
	FILE *fh;
	CURL *c;

	debug( "Opening config url '%s', section %s",
		url, context->section->name );

	// set up our new context
	if( !( c = curl_easy_init( ) ) )
	{
		err( "Could not init curl for url fetch -- %s", Err );
		return -1;
	}

	curl_easy_setopt( c, CURLOPT_URL, url );
	curl_easy_setopt( c, CURLOPT_TIMEOUT_MS, 10000 );
	curl_easy_setopt( c, CURLOPT_CONNECTTIMEOUT_MS, 5000 );

	if( context->is_ssl )
	{
		if( chkcfFlag( SEC_VALIDATE ) )
		{
			curl_easy_setopt( c, CURLOPT_SSL_VERIFYPEER,   1L );
			curl_easy_setopt( c, CURLOPT_SSL_VERIFYHOST,   1L );
			curl_easy_setopt( c, CURLOPT_SSL_VERIFYSTATUS, 1L );
		}
		else
		{
			curl_easy_setopt( c, CURLOPT_SSL_VERIFYPEER,   0L );
			curl_easy_setopt( c, CURLOPT_SSL_VERIFYHOST,   0L );
			curl_easy_setopt( c, CURLOPT_SSL_VERIFYSTATUS, 0L );
		}
	}

	// make a temporary file
	if( !( fh = tmpfile( ) ) )
	{
		err( "Could not create a temporary file for fetching '%s' -- %s",
			url, Err );
		ret = -2;
		goto CRU_CLEANUP;
	}

	// and set the file handle
	curl_easy_setopt( c, CURLOPT_WRITEDATA, (void *) fh );

	// and go get it
	if( ( cc = curl_easy_perform( c ) ) != CURLE_OK )
	{
		err( "Could not curl target url '%s' -- %s", url, CErr );
		ret = -1;
		goto CRU_CLEANUP;
	}

	// and go back to the beginning
	fseek( fh, 0L, SEEK_SET );

	// and read that file
	ret = __config_read_file( fh );

CRU_CLEANUP:
	curl_easy_cleanup( c );
	return ret;
}


#undef CErr


int config_read( char *inpath, WORDS *w )
{
	int ret = 0, p_url = 0, p_ssl = 0;
	char *path;

	// prune the path
	path = config_relative_path( inpath );

	// check this isn't a source loop
	if( config_source_dupe( path ) )
	{
		warn( "Skipping duplicate config source '%s'.", path );
		ret = _proc->strict;
		goto Read_Done;
	}

	// set up our new context
	context = config_make_context( path, w );

	if( !strncmp( path, "http://", 7 ) )
		context->is_url = 1;
	else if( !strncmp( path, "https://", 8 ) )
	{
		context->is_url = 1;
		context->is_ssl = 1;
	}

	// do we have a parent context?
	if( context->parent )
	{
		p_url = context->parent->is_url;
		p_ssl = context->parent->is_ssl;
	}

	// read checks
	if( context->is_url )
	{
		// do we allow urls to include urls?
		if( !chkcfFlag( READ_URL ) )
		{
			warn( "Skipping URL source '%s' due to config flags.", path );
			ret = _proc->strict;
			goto Read_Done;
		}

		if( !context->is_ssl && !chkcfFlag( URL_INSEC ) )
		{
			warn( "Skipping insecure URL source '%s' due to config flags", path );
			ret = _proc->strict;
			goto Read_Done;
		}

		if( p_url && !chkcfFlag( URL_INC_URL ) )
		{
			warn( "Skipping URL-included URL '%s' due to config flags.", path );
			ret = _proc->strict;
			goto Read_Done;
		}

		// do we allow secure urls to include insecure ones?
		if( p_ssl && !context->is_ssl && !chkcfFlag( SEC_INC_INSEC ) )
		{
			warn( "Skipping insecure URL '%s' included from secure URL.", path );
			ret = _proc->strict;
			goto Read_Done;
		}
	}
	// do we allow files?
	else if( !chkcfFlag( READ_FILE ) )
	{
		warn( "Skipping file source '%s' due to config flags.", path );
		ret = _proc->strict;
		goto Read_Done;
	}

	// so go do it
	if( context->is_url )
		ret = config_read_url( path );
	else
		ret = config_read_file( path );

Read_Done:
	free( path );
	return ret;
}



void config_choose_section( CCTXT *c, char *section )
{
	int i;

	for( i = 0; i < CONF_SECT_MAX; i++ )
	{
		if( !config_sections[i].name )
			break;

		if( !strcasecmp( config_sections[i].name, section ) )
		{
			c->section = &(config_sections[i]);
			return;
		}
	}

	// default to main
	c->section = &(config_sections[0]);
}



int config_env_path( char *path, int len )
{
	char *sec, *pth, *us, *p;
	AVP av;

	// catch the config file entry - not part of the
	// regular env processing
	if( !strncasecmp( path, "FILE=", 5 ) )
	{
		if( var_val( path, len, &av, CFG_VV_FLAGS ) )
		{
			err( "Could not process env path." );
			return -1;
		}

		// command line file beats environment setting
		if( chkcfFlag( FILE_OPT ) )
			info( "Environment-set config flag ignored in favour of run option." );
		else
			snprintf( _proc->cfg_file, CONF_LINE_MAX, "%s", av.val );
		return 0;
	}


	if( path[0] == '_' )
	{
		// 'main' section
		sec = "";
		pth = path + 1;
		len--;
	}
	else if( !( us = memchr( path, '_', len ) ) )
	{
		warn( "No section found in env path %s", path );
		return -1;
	}
	else
	{
		sec  = path;
		*us  = '\0';
		pth  = ++us;
		len -= pth - path;

		// lower-case the section
		for( p = path; p < us; p++ )
			*p = tolower( *p );

		// we have to swap any _ for . in the config path here because
		// env names cannot have dots in
		for( p = pth; *p; p++ )
			if( *p == '_' )
				*p = '.';

	}

	// this lowercases it
	if( var_val( pth, len, &av, CFG_VV_FLAGS ) )
	{
		warn( "Could not process env path." );
		return -1;
	}

	// pick section
	config_choose_section( context, sec );

	// and try it
	if( (*(context->section->fp))( &av ) )
	{
		debug( "Found env [%s] %s -> %s", sec, av.att, av.val );
		return 0;
	}

	return -1;
}


#define ENV_MAX_LENGTH			65536

int config_read_env( char **env )
{
	char buf[ENV_MAX_LENGTH];
	int l;

	// config flag disables this
	if( !chkcfFlag( READ_ENV ) )
	{
		debug( "No reading environment due to config flags." );
		return 0;
	}

	context = config_make_context( "environment", NULL );

	for( ; *env; env++ )
	{
		l = snprintf( buf, ENV_MAX_LENGTH, "%s", *env );

		if( l < ( _proc->env_prfx_len + 2 ) || memcmp( buf, _proc->env_prfx, _proc->env_prfx_len ) )
			continue;

		if( config_env_path( buf + _proc->env_prfx_len, l - _proc->env_prfx_len ) )
		{
			warn( "Problematic environmental config item %s", *env );
			return -1;
		}
	}

	return 0;
}

#undef ENV_MAX_LENGTH


void config_set_main_file( char *path )
{
	snprintf( _proc->cfg_file, CONF_LINE_MAX, "%s", path );

	// this wins against env
	setcfFlag( FILE_OPT );
}

void config_set_env_prefix( char *prefix )
{
	char *p, prev;
	int i;

	// it needs to be uppercase
	for( i = 0, p = prefix; i < 125 && *p; p++ )
	{
		_proc->env_prfx[i++] = toupper( *p );
		prev = *p;
	}

	// and end with _
	if( prev != '_' )
		_proc->env_prfx[i++] = '_';

	_proc->env_prfx[i]  = '\0';
	_proc->env_prfx_len = i;
}

void config_set_pid_file( char *path )
{
	snprintf( _proc->pidfile, CONF_LINE_MAX, "%s", path );
}


void config_register_section( char *name, conf_line_fn *fp )
{
	CSECT *s = &(config_sections[_proc->sect_count]);

	s->name    = str_dup( name, 0 );
	s->fp      = fp;
	s->section = _proc->sect_count;

	_proc->sect_count++;
}


char *config_help( void )
{
	return "\
 -h            Print this help\n\
 -v            Print version number and exit\n\
 -t            Just test the config is valid and exit\n\
 -c <path>     Select config file or URI to use\n\
 -C <path>     As -c but with strict processing\n\
 -E            Disable reading environment variables\n\
 -P <prefix>   Set environment variable prefix\n\
 -F            Disable reading a config file (env only)\n\
 -U            Disable all reading of URI's\n\
 -u            Disable URI config including other URI's\n\
 -i            Allow insecure URI's\n\
 -I            Allow secure URI's to include insecure URI's\n\
 -T            Validate certificates from config hosts\n\
 -d            Daemonize in the background\n\
 -D            Switch on debug output (overrides config)\n\
 -V            Logging to console (prevents daemonizing)\n\
 -s            Strict config parsing\n\
";
}


const char *config_args_opt_string = "HhDVvtsUuiIETFdC:c:";
char config_args_opt_merged[CONF_LINE_MAX];

char *config_arg_string( char *argstr )
{
	snprintf( config_args_opt_merged, CONF_LINE_MAX, "%s%s", config_args_opt_string, argstr );
	return config_args_opt_merged;
}



void config_args( int ac, char **av, char *optstr, help_fn *hfp )
{
	int oc;

	while( ( oc = getopt( ac, av, optstr ) ) != -1 )
		switch( oc )
		{
			case 'H':
			case 'h':
				(*hfp)( );
				break;
			case 'C':
				_proc->strict = -1;
				config_set_main_file( optarg );
				break;
			case 'c':
				config_set_main_file( optarg );
				break;
			case 'd':
				runf_add( RUN_DAEMON );
				break;
			case 'D':
				_logger->level = LOG_LEVEL_DEBUG;
				runf_add( RUN_DEBUG );
				break;
			case 'V':
				runf_add( RUN_TGT_STDOUT );
				_logger->force_stdout = 1;
				break;
			case 'v':
				printf( "%s version: %s\n", _proc->app_upper, _proc->version );
				exit( 0 );
				break;
			case 't':
				setcfFlag( TEST_ONLY );
				break;
			case 's':
				_proc->strict = -1;
				break;
			case 'P':
				config_set_env_prefix( optarg );
				break;
			case 'U':
				cutcfFlag( URL_INC_URL );
				cutcfFlag( READ_URL );
				break;
			case 'u':
				cutcfFlag( URL_INC_URL );
				break;
			case 'i':
				setcfFlag( URL_INSEC );
				break;
			case 'I':
				setcfFlag( SEC_INC_INSEC );
				break;
			case 'T':
				setcfFlag( SEC_VALIDATE );
				break;
			case 'E':
				cutcfFlag( READ_ENV );
				break;
			case 'F':
				cutcfFlag( READ_FILE );
				break;
		}

	// reset arg processing
	optind = 1;
}




PROC_CTL *config_defaults( char *app_name )
{
	_proc            = (PROC_CTL *) allocz( sizeof( PROC_CTL ) );
	_proc->version   = strdup( VERSION_STRING );
	_proc->app_name  = strdup( app_name );
	_proc->tick_usec = 1000 * DEFAULT_TICK_MSEC;

	snprintf( _proc->app_upper, CONF_LINE_MAX, "%s", app_name );
	_proc->app_upper[0] = toupper( _proc->app_upper[0] );

	snprintf( _proc->basedir,  CONF_LINE_MAX, "/etc/ministry" );
	snprintf( _proc->cfg_file, CONF_LINE_MAX, "/etc/ministry/%s.conf", app_name );
	snprintf( _proc->pidfile,  CONF_LINE_MAX, "/var/run/ministry/%s.pid", app_name );

	config_set_env_prefix( app_name );

	// we like adaptive mutexes please
	pthread_mutexattr_init( &(_proc->mtxa) );
#ifdef DEFAULT_MUTEXES
	pthread_mutexattr_settype( &(_proc->mtxa), PTHREAD_MUTEX_DEFAULT );
#else
	pthread_mutexattr_settype( &(_proc->mtxa), PTHREAD_MUTEX_ADAPTIVE_NP );
#endif

	// max these two out
	config_set_limit( _proc, RLIMIT_NOFILE, -1 );
	config_set_limit( _proc, RLIMIT_NPROC,  -1 );

	clock_gettime( CLOCK_REALTIME, &(_proc->init_time) );
	tsdupe( _proc->init_time, _proc->curr_time );

	// initial conf flags
	XsetcfFlag( _proc, READ_FILE );
	XsetcfFlag( _proc, READ_ENV );
	XsetcfFlag( _proc, READ_URL );
	XsetcfFlag( _proc, URL_INC_URL );
	// but not: sec include non-sec, read non-sec, validate

	return _proc;
}

#undef config_set_limit

