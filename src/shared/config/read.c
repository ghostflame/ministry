/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* config/read.c - read config files                                       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#include "local.h"


CSECT config_sections[CONF_SECT_MAX];


/*
 * Matches arg substitutions of the form:
 * %N% where N <= argc
 */
int config_args_substitute( AVP *av )
{
	if( !context->argc || av->vlen < 3 )
		return 0;

	return str_sub( &(av->vptr), &(av->vlen), context->argc, context->argv, context->argl );
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
		++(context->lineno);

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
			if( config_args_substitute( av ) > 0 )
				av->doFree = 1;

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




int __config_handle_line( AVP *av )
{
	int ret = 0;

	// spot includes - they inherit context
	if( !strcasecmp( av->aptr, "include" ) )
	{
		WORDS w;

		if( !chkcfFlag( READ_INCLUDE ) )
		{
			warn( "Ignoring include directive due to --no-include option." );
			return 0;
		}

		strwords( &w, av->vptr, av->vlen, ' ' );

		if( config_read( w.wd[0], &w ) != 0 )
		{
			err( "Included config file '%s' is not valid.", av->vptr );
			return -1;
		}

		return 0;
	}

	debug( "Config line: %s = %s", av->aptr, av->vptr );

	// then dispatch to the correct handler
	ret = (*(context->section->fp))( av );

	if( av->doFree )
		free( av->vptr );

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

		// repeat the next line based on other args
		if( !strcasecmp( av.aptr, "foreach" ) )
		{
			ITER *it = iter_init( NULL, av.vptr, av.vlen );
			char *vcpy, *vtmp, *arg;
			int alen, vlen;

			// get our next line
			if( ( rv = config_get_line( fh, &av ) ) )
				break;

			// we need two copies because processing
			// may alter the input string, and we need
			// to keep replaying it
			vlen = av.vlen;
			vcpy = (char *) allocz( vlen + 2 );
			vtmp = (char *) allocz( vlen + iter_longest( it ) + 2 );

			memcpy( vcpy, av.vptr, av.vlen );
			vcpy[av.vlen] = '\0';

			// blanks have an autoassigned '1' we want to ignore
			// if you really want a 1 with arguments, put a 1
			if( av.blank )
				vlen = 0;
			else
			 	vtmp[vlen++] = ' ';

			// run while arguments...
			while( iter_next( it, &arg, &alen ) == 0 )
			{
				// copy the base - might be 0, might be base + space
				memcpy( vtmp, vcpy, vlen );
				// add the latest arg
				memcpy( vtmp + vlen, arg, alen );

				// point the av struct at it
				av.vptr = vtmp;
				av.vlen = vlen + alen;

				// and cap it
				vtmp[av.vlen] = '\0';

				debug( "Config iterator calling: %s = %s", av.aptr, av.vptr );

				lrv = __config_handle_line( &av );

				ret += lrv;
				if( lrv )
				{
					err( "Bad config in file '%s', line %d (repeat arg %s)",
						context->source, context->lineno, arg );
					info( "Bad line: %s = %s", av.aptr, av.vptr );
					break;
				}
			}

			iter_clean( it, 1 );
			free( vcpy );
			free( vtmp );

			if( ret )
				break;
		}
		else
		{
			lrv = __config_handle_line( &av );
			ret += lrv;

			if( lrv )
			{
				err( "Bad config in file '%s', line %d", context->source, context->lineno );
				info( "Bad line: %s = %s", av.aptr, av.vptr );
				break;
			}
		}
	}

	if( fh )
		fclose( fh );

	debug( "Finished with config source '%s': %d", context->source, ret );

	// step our context back
	context = context->parent;

	return ret;
}





int config_read_file( char *path, int fail_ok )
{
	FILE *fh = NULL;

	// die on not reading main config file, warn on others
	if( !( fh = fopen( path, "r" ) ) )
	{
		if( fail_ok )
		{
			notice( "Could not open optional config file '%s' -- %s", path, Err );
			return 0;
		}
		else
		{
			err( "Could not open config file '%s' -- %s", path, Err );
			return -1;
		}
	}

	return __config_read_file( fh );
}





int config_read_url( char *url, int fail_ok )
{
	CURLWH ch;
	int ret;

	memset( &ch, 0, sizeof( CURLWH ) );

	ch.url = url;

	if( context->is_ssl )
		setCurlFl( ch, SSL );

	if( chkcfFlag( SEC_VALIDATE ) )
		setCurlFl( ch, VALIDATE );

	// force to file
	setCurlFl( ch, TOFILE );

	if( curlw_fetch( &ch ) )
	{
		if( fail_ok )
		{
			warn( "Could not fetch optional target url '%s'", url );
			return 0;
		}
		else
		{
			err( "Could not fetch target url '%s'", url );
			return -1;
		}
	}

	// and read that file
	ret = __config_read_file( ch.fh );
	fclose( ch.fh );

	return ret;
}


#undef CErr


int config_read( char *inpath, WORDS *w )
{
	int ret = 0, p_url = 0, p_ssl = 0, s, fail_ok = 0;
	char *path;

	// are we allowing this include to fail?
	if( *inpath == '?' )
	{
		fail_ok = 1;
		++inpath;
	}

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

	// is that a url?
	s = is_url( path );

	if( s == STR_URL_YES )
		context->is_url = 1;
	else if( s == STR_URL_SSL )
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

	debug( "Opening config %s '%s', section '%s', %d arg%s.",
		( context->is_url ) ? "url" : "file", path,
		context->section->name, context->argc,
		VAL_PLURAL( context->argc ) );

	// so go do it
	if( context->is_url )
		ret = config_read_url( path, fail_ok );
	else
		ret = config_read_file( path, fail_ok );

Read_Done:
	free( path );
	return ret;
}



void config_choose_section( CCTXT *c, char *section )
{
	int i;

	for( i = 0; i < CONF_SECT_MAX; ++i )
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




