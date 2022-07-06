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
* config/config.c - read config files and create config object            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#include "local.h"



int config_on_change( FTREE *ft, uint32_t mask, const char *path, const char *desc, void *arg )
{
	// notify that we have changed - no locking needed, only one thread watches config
	++(_proc->cfgChanged);

	if( XchkcfFlag( _proc, CHG_EXIT ) )
	{
		warn( "Config path %s has registered a change 0x%08x (%s), exiting.", path, mask, desc );

		if( RUNNING( ) )
		{
			RUN_STOP( );
		}
		else
		{
			// we are out
			app_finish( 0 );
		}
	}
	else
	{
		warn( "Config path %s has registered a change 0x%08x (%s).", path, mask, desc );
	}

	return 0;
}



int config_bool( AVP *av )
{
	if( valIs( "true" ) || valIs( "yes" ) || valIs( "y" ) || ( strtol( av->vptr, NULL, 10 ) != 0 ) )
		return 1;

	return 0;
}



#define config_set_limit( _p, _r, _v )	\
(_p)->limits[_r] = _v;\
(_p)->setlim[_r] = 0x01


int config_line( AVP *av )
{
	int64_t v;
	char *d;
	int res;

	if( ( d = strchr( av->aptr, '.' ) ) )
	{
		++d;

		if( !strncasecmp( av->aptr, "limits.", 7 ) )
		{
			if( !strcasecmp( d, "procs" ) )
				res = RLIMIT_NPROC;
			else if( !strcasecmp( d, "files" ) )
				res = RLIMIT_NOFILE;
			else
				return -1;

			if( av_int( v ) == NUM_INVALID )
			{
				warn( "Invalid limits value: %s", av->vptr );
				return -1;
			}

			config_set_limit( _proc, res, v );
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
	else if( attIs( "tmpdir" ) )
	{
		free( _proc->tmpdir );
		_proc->tmpdir = str_copy( av->vptr, av->vlen );
	}
	else if( attIs( "maxJsonSz" ) )
	{
		_proc->max_json_sz = strtol( av->vptr, NULL, 10 );
	}
	else if( attIs( "pidFile" ) )
	{
		config_set_pid_file( av->vptr );
	}
	else if( attIs( "baseDir" ) )
	{
		config_set_basedir( av->vptr );
	}
	else if( attIs( "exitOnChange" ) )
	{
		if( config_bool( av ) )
			XsetcfFlag( _proc, CHG_EXIT );
		else
			XcutcfFlag( _proc, CHG_EXIT );
	}

	return 0;
}




void config_set_pid_file( const char *path )
{
	if( _proc->pidfile )
		free( _proc->pidfile );

	_proc->pidfile = str_copy( path, 0 );
}

void config_set_basedir( const char *dir )
{
	if( _proc->basedir )
		free( _proc->basedir );

	_proc->basedir = str_copy( dir, 0 );
}



void config_late_setup( void )
{
	// and watch our config files
	_proc->cfiles = fs_treemon_create( NULL, NULL, &config_on_change, NULL );
}


void config_register_section( const char *name, conf_line_fn *fp )
{
	CSECT *s = &(config_sections[_proc->sect_count]);

	s->name    = str_perm( name, 0 );
	s->fp      = fp;
	s->section = _proc->sect_count;

	++(_proc->sect_count);
}



PROC_CTL *config_defaults( const char *app_name, const char *conf_dir )
{
	char buf[1024];

	_proc            = (PROC_CTL *) mem_perm( sizeof( PROC_CTL ) );
	_proc->version   = str_perm( VERSION_STRING, 0 );
	_proc->app_name  = str_perm( app_name, 0 );
	_proc->tick_usec = 1000 * DEFAULT_TICK_MSEC;

	if( gethostname( buf, 1024 ) )
	{
		fatal( "Cannot get my own hostname -- %s.", Err );
		return NULL;
	}
	// just in case
	buf[1023] = '\0';
	_proc->hostname = str_perm( buf, 0 );

	// prefer .conf files in includes
	config_set_suffix( ".conf" );

	_proc->app_upper = str_perm( app_name, 0 );
	_proc->app_upper[0] = toupper( _proc->app_upper[0] );

	snprintf( buf, 1024, "/etc/%s/%s.conf", conf_dir, app_name );
	config_set_main_file( buf );

	snprintf( buf, 1024, "/var/run/%s/%s.pid", conf_dir, app_name );
	config_set_pid_file( buf );

	snprintf( buf, 1024, "/etc/%s", conf_dir );
	config_set_basedir( buf );

	_proc->tmpdir = str_copy( TMP_DIR, 0 );
	_proc->max_json_sz = MAX_JSON_SZ;

	config_set_env_prefix( app_name );

	// max these two out
	config_set_limit( _proc, RLIMIT_NOFILE, -1 );
	config_set_limit( _proc, RLIMIT_NPROC,  -1 );

	clock_gettime( CLOCK_REALTIME, &(_proc->init_time) );
	tsdupe( _proc->init_time, _proc->curr_time );

	// initial conf flags
	XsetcfFlag( _proc, READ_FILE );
	XsetcfFlag( _proc, READ_ENV );
	XsetcfFlag( _proc, READ_URL );
	XsetcfFlag( _proc, READ_INCLUDE );
	XsetcfFlag( _proc, URL_INC_URL );
	XsetcfFlag( _proc, SUFFIX );
	// but not: sec include non-sec, read non-sec, validate

	// make sure our fixed config sections array is clean
	memset( config_sections, 0, CONF_SECT_MAX * sizeof( CSECT ) );

	// init the lock
	pthread_mutex_init( &(_proc->cfg_lock), NULL );

	return _proc;
}

#undef config_set_limit

