/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* config/config.c - read config files and create config object            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#include "local.h"


void config_check_times( int64_t tval, void *arg )
{
	struct stat sb;
	int64_t mt;
	CFILE *cf;

	for( cf = _proc->cfiles; cf; cf = cf->next )
	{
		if( stat( cf->fpath, &sb ) )
		{
			if( ++(cf->fcount) >= 3 )
			{
				err( "Failed (3 times) to stat config file %s -- %s", cf->fpath, Err );
				RUN_STOP( );
			}
			else
				warn( "Failed to stat config file %s -- %s", cf->fpath, Err );
		}
		else
		{
			cf->fcount = 0;
			mt = tsll( sb.st_mtim );

			if( mt > cf->mtime )
			{
				mt -= cf->mtime;
				mt /= BILLION;

				notice( "Config file %s mtime has changed (+%lld sec).",
					cf->fpath, mt );

				RUN_STOP( );
			}
			else
				debug( "Config file %s has not changed.", cf->fpath );
		}
	}
}


void config_monitor( THRD *t )
{
	_proc->cf_chk_time *= MILLION;

	notice( "Monitoring %d config file%s for changes.",
			_proc->cf_chk_ct, ( _proc->cf_chk_ct == 1 ) ? "" : "s" );

	loop_control( "conf_monitor", &config_check_times, NULL, _proc->cf_chk_time, LOOP_TRIM, 0 );
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
		snprintf( _proc->pidfile, CONF_LINE_MAX, "%s", av->vptr );
	}
	else if( attIs( "baseDir" ) )
	{
		snprintf( _proc->basedir, CONF_LINE_MAX, "%s", av->vptr );
	}
	else if( attIs( "configMonitorSec" ) )
	{
		av_int( _proc->cf_chk_time );
		debug( "Set config check time to %lld sec.", _proc->cf_chk_time );
	}

	return 0;
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

	++(_proc->sect_count);
}



PROC_CTL *config_defaults( char *app_name, char *conf_dir )
{
	char buf[1024];

	_proc            = (PROC_CTL *) allocz( sizeof( PROC_CTL ) );
	_proc->version   = strdup( VERSION_STRING );
	_proc->app_name  = strdup( app_name );
	_proc->tick_usec = 1000 * DEFAULT_TICK_MSEC;

	if( gethostname( buf, 1024 ) )
	{
		fatal( "Cannot get my own hostname -- %s.", Err );
		return NULL;
	}
	// just in case
	buf[1023] = '\0';
	_proc->hostname = str_dup( buf, 0 );

	// prefer .conf files in includes
	config_set_suffix( ".conf" );

	snprintf( _proc->app_upper, CONF_LINE_MAX, "%s", app_name );
	_proc->app_upper[0] = toupper( _proc->app_upper[0] );

	snprintf( _proc->basedir,  CONF_LINE_MAX, "/etc/%s", conf_dir );
	snprintf( _proc->cfg_file, CONF_LINE_MAX, "/etc/%s/%s.conf", conf_dir, app_name );
	snprintf( _proc->pidfile,  CONF_LINE_MAX, "/var/run/%s/%s.pid", conf_dir, app_name );

	_proc->tmpdir = str_copy( TMP_DIR, 0 );
	_proc->max_json_sz = MAX_JSON_SZ;

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
	XsetcfFlag( _proc, READ_INCLUDE );
	XsetcfFlag( _proc, URL_INC_URL );
	XsetcfFlag( _proc, SUFFIX );
	// but not: sec include non-sec, read non-sec, validate

	// make sure our fixed config sections array is clean
	memset( config_sections, 0, CONF_SECT_MAX * sizeof( CSECT ) );

	return _proc;
}

#undef config_set_limit

