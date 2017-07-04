/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* config.c - read config files and create config object                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#include "ministry_test.h"


CCTXT *ctxt_top = NULL;
CCTXT *context  = NULL;

#define CFG_VV_FLAGS	(VV_AUTO_VAL|VV_LOWER_ATT|VV_REMOVE_UDRSCR)

// read a file until we find a config line
char __cfg_read_line[1024];
int config_get_line( FILE *f, AVP *av )
{
	char *n, *e;

	while( 1 )
	{
		// end things on a problem
		if( !f || feof( f ) || ferror( f ) )
			break;
		if( !fgets( __cfg_read_line, 1024, f ) )
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

			// grab that
			snprintf( context->section, 512, "%s", __cfg_read_line + 1 );

			// spotted a section
			return 1;
		}

		// returns 0 if it can cope
		if( var_val( __cfg_read_line, n - __cfg_read_line, av, CFG_VV_FLAGS ) )
			continue;

		// return any useful data
		if( av->status == VV_LINE_ATTVAL )
			return 0;
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



CCTXT *config_make_context( char *path, CCTXT *parent )
{
	CCTXT *ctx;

	ctx = (CCTXT *) allocz( sizeof( CCTXT ) );
	snprintf( ctx->source, 4096, "%s", path );

	if( parent )
	{
		ctx->parent = parent;

		if( parent->children )
			parent->children->next = ctx;
		parent->children = ctx;

		// inherit the section we were in
		memcpy( ctx->section, parent->section, 512 );
	}
	else
		strncpy( ctx->section, "main", 6 );

	// set the global on the first call
	if( !ctxt_top )
		ctxt_top = ctx;

	return ctx;
}



int config_source_dupe( CCTXT *c, char *path )
{
	CCTXT *ch;

	if( !c )
		return 0;

	if( !strcmp( path, c->source ) )
		return 1;

	for( ch = c->children; ch; ch = ch->next )
		if( config_source_dupe( ch, path ) )
			return 1;

	return 0;
}



#define config_set_limit( _c, _r, _v )	\
(_c)->limits[_r] = (int64_t) _v;\
(_c)->setlim[_r] = 0x01


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

			config_set_limit( ctl, res, atoll( av->val ) );
		}
		else
			return -1;

		return 0;
	}

	if( attIs( "tickMsec" ) )
	{
		av_int( ctl->tick_usec );
		ctl->tick_usec *= 1000;
	}
	else if( attIs( "daemon" ) )
	{
		if( config_bool( av ) )
			ctl->run_flags |= RUN_DAEMON;
		else
			ctl->run_flags &= ~RUN_DAEMON;
	}
	else if( attIs( "baseDir" ) )
	{
		free( ctl->basedir );
		ctl->basedir = str_copy( av->val, av->vlen );
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

	if( !ctl->basedir )
		// just step over it
		return strdup( inpath + 1 );

	// add 1 for the /, remove 1 for the ~, add 1 for the \0
	len = strlen( ctl->basedir ) + strlen( inpath ) + 1;

	ret = (char *) malloc( len );
	snprintf( ret, len, "%s/%s", ctl->basedir, inpath + 1 );

	return ret;
}


int __config_read_file( FILE *fh )
{
	int rv, lrv, ret = 0;
	AVP av;

	while( ( rv = config_get_line( fh, &av ) ) != -1 )
	{
		// was it just a section change?
		if( rv > 0 )
			continue;

		// spot includes - they inherit context
		if( !strcasecmp( av.att, "include" ) )
		{
			if( config_read( av.val ) != 0 )
			{
				err( "Included config file '%s' is not valid.", av.val );
				ret = -1;
				break;
			}
			continue;
		}

		// and dispatch it
		lrv = config_choose_handler( context->section, &av );
		ret += lrv;

		if( lrv )
		{
			err( "Bad config in file '%s', line %d", context->source, context->lineno );
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
		path, context->section );

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
		url, context->section );

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
		curl_easy_setopt( c, CURLOPT_SSL_VERIFYPEER, 0L );
		curl_easy_setopt( c, CURLOPT_SSL_VERIFYHOST, 0L );
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


int config_read( char *inpath )
{
	int ret = 0, p_url = 0, p_ssl = 0;
	char *path;

	// prune the path
	path = config_relative_path( inpath );

	// check this isn't a duplicate
	if( config_source_dupe( ctxt_top, path ) )
	{
		warn( "Skipping duplicate config source '%s'.", path );
		ret = ctl->strict;
		goto Read_Done;
	}

	// set up our new context
	context = config_make_context( path, context );

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
			ret = ctl->strict;
			goto Read_Done;
		}

		if( !context->is_ssl && !chkcfFlag( URL_INSEC ) )
		{
			warn( "Skipping insecure URL source '%s' due to config flags", path );
			ret = ctl->strict;
			goto Read_Done;
		}

		if( p_url && !chkcfFlag( URL_INC_URL ) )
		{
			warn( "Skipping URL-included URL '%s' due to config flags.", path );
			ret = ctl->strict;
			goto Read_Done;
		}

		// do we allow secure urls to include insecure ones?
		if( p_ssl && !context->is_ssl && !chkcfFlag( SEC_INC_INSEC ) )
		{
			warn( "Skipping insecure URL '%s' included from secure URL.", path );
			ret = ctl->strict;
			goto Read_Done;
		}
	}
	// do we allow files?
	else if( !chkcfFlag( READ_FILE ) )
	{
		warn( "Skipping file source '%s' due to config flags.", path );
		ret = ctl->strict;
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


#define	secIs( s )		!strcasecmp( section, s )

int config_choose_handler( char *section, AVP *av )
{
	// hand some sections off to different config fns
	if( secIs( "logging" ) )
		return log_config_line( av );
	else if( secIs( "memory" ) )
		return mem_config_line( av );
	else if( secIs( "metric" ) )
		return metric_config_line( av );
	else if( secIs( "target" ) )
		return target_config_line( av );

	return config_line( av );
}

#undef secIs


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
		{
			free( ctl->cfg_file );
			ctl->cfg_file = strdup( av.val );
		}
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

	// and try it
	if( !config_choose_handler( sec, &av ) )
	{
		debug( "Found env [%s] %s -> %s", sec, av.att, av.val );
		return 0;
	}

	return -1;
}


#define ENV_MAX_LENGTH			65536
#define ENV_PREFIX				"MINISTRY_TEST_"
#define ENV_PREFIX_LEN			14	// length of the above

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

	for( ; *env; env++ )
	{
		l = snprintf( buf, ENV_MAX_LENGTH, "%s", *env );

		if( l < 14 || memcmp( buf, ENV_PREFIX, ENV_PREFIX_LEN ) )
			continue;

		if( config_env_path( buf + ENV_PREFIX_LEN, l - ENV_PREFIX_LEN ) )
		{
			warn( "Problematic environmental config item %s", *env );
			return -1;
		}
	}

	return 0;
}

#undef ENV_MAX_LENGTH
#undef ENV_PREFIX
#undef ENV_PREFIX_LEN


void config_create( void )
{
	ctl             = (MTEST_CTL *) allocz( sizeof( MTEST_CTL ) );

	ctl->locks      = lock_config_defaults( );
	ctl->log        = log_config_defaults( );
	ctl->mem        = mem_config_defaults( );
	ctl->metric     = metric_config_defaults( );
	ctl->tgt        = target_config_defaults( );

	ctl->cfg_file   = strdup( DEFAULT_CONFIG_FILE );
	ctl->version    = strdup( MINISTRY_TEST_VERSION );
	ctl->basedir    = strdup( DEFAULT_BASE_DIR );

	ctl->tick_usec  = 1000 * DEFAULT_TICK_MSEC;

	// max these two out
	config_set_limit( ctl, RLIMIT_NOFILE, -1 );
	config_set_limit( ctl, RLIMIT_NPROC,  -1 );

	clock_gettime( CLOCK_REALTIME, &(ctl->init_time) );
	tsdupe( ctl->init_time, ctl->curr_time );

	// initial conf flags
	setcfFlag( READ_FILE );
	setcfFlag( READ_ENV );
	setcfFlag( READ_URL );
	setcfFlag( URL_INC_URL );
	// but not: sec include non-sec, read non-sec
}

#undef config_set_limit

