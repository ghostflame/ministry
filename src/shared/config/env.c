/**************************************************************************
* Copyright 2015 John Denholm                                             *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
*                                                                         *
*                                                                         *
* config/env.c - read config values from the environment                  *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#include "local.h"



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
		for( p = path; p < us; ++p )
			*p = tolower( *p );

		// we have to swap any _ for . in the config path here because
		// env names cannot have dots in
		for( p = pth; *p; ++p )
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
		return -1;

	debug( "Found env [%s] %s -> %s", sec, av.att, av.val );
	return 0;
}


#define ENV_MAX_LENGTH			65536

int config_read_env( const char **env )
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

	for( ; *env; ++env )
	{
		l = snprintf( buf, ENV_MAX_LENGTH, "%s", *env );

		if( l < ( _proc->env_prfx_len + 2 ) || memcmp( buf, _proc->env_prfx, _proc->env_prfx_len ) )
			continue;

		notice( "Env Entry: %s", buf );

		if( config_env_path( buf + _proc->env_prfx_len, l - _proc->env_prfx_len ) )
		{
			warn( "Problematic environmental config item %s", *env );
			return -1;
		}
	}

	return 0;
}

#undef ENV_MAX_LENGTH


