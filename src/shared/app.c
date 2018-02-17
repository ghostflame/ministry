/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* app.c - basic app startup and shutdown                                  *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "shared.h"

PROC_CTL *_proc = NULL;
LOG_CTL *_logger = NULL;
MEM_CTL *_mem = NULL;
HTTP_CTL *_http = NULL;


void catch_pipe( int sig )
{
	debug( "Caught SIGPIPE." );
}

int set_signals( void )
{
	struct sigaction sa;

	memset( &sa, 0, sizeof( struct sigaction ) );
	sa.sa_handler = loop_kill;
	sa.sa_flags   = SA_NOMASK;

	// finish signal
	if( sigaction( SIGTERM, &sa, NULL )
	 || sigaction( SIGQUIT, &sa, NULL )
	 || sigaction( SIGINT,  &sa, NULL ) )
	{
		err( "Could not set exit signal handlers -- %s", Err );
		return -1;
	}

	// log bounce
	sa.sa_handler = log_reopen;
	if( sigaction( SIGHUP, &sa, NULL ) )
	{
		err( "Could not set hup signal handler -- %s", Err );
		return -2;
	}

	// and sigpipe
	sa.sa_handler = catch_pipe;
	if( sigaction( SIGPIPE, &sa, NULL ) )
	{
		err( "Could not set pipe signal handler -- %s", Err );
		return -3;
	}

	return 0;
}


int set_limits( void )
{
	int i, ret = 0;

	for( i = 0; i < RLIMIT_NLIMITS; i++ )
		if( _proc->setlim[i] )
			ret += setlimit( i, _proc->limits[i] );

	return ret;
}





int app_init( char *name )
{
	// create some stuff
	config_defaults( name );

	log_config_defaults( );
	mem_config_defaults( );
	http_config_defaults( );
	io_config_defaults( );
	iplist_config_defaults( );
	target_config_defaults( );

	// set up our shared config
	memset( config_sections, 0, CONF_SECT_MAX * sizeof( CSECT ) );
	config_register_section( "main",     &config_line );
	config_register_section( "logging",  &log_config_line );
	config_register_section( "memory",   &mem_config_line );
	config_register_section( "http",     &http_config_line );
	config_register_section( "iplist",   &iplist_config_line );
	config_register_section( "io",       &io_config_line );
	config_register_section( "target",   &target_config_line );

	if( set_signals( ) )
	{
		err( "Could not set signals." );
		app_finish( 2 );
	}

	pthread_mutex_init( &(_proc->loop_lock), NULL );

	// make curl ready
	curl_global_init( CURL_GLOBAL_SSL );

	runf_add( RUN_APP_INIT );

	return 0;
}


int app_start( int writePid )
{
	if( !runf_has( RUN_APP_INIT ) )
		fatal( "App has not been init'd yet." );

	curl_global_cleanup( );

	if( chkcfFlag( TEST_ONLY ) )
	{
		printf( "Config file '%s' is OK.\n", _proc->cfg_file );
		exit( 0 );
	}

	if( set_limits( ) )
		fatal( "Failed to set limits." );

	if( !_logger->force_stdout && !_logger->use_std )
		debug( "Starting logging - no more logs to stdout." );

	log_start( );

	notice( "%s v%s starting up.", _proc->app_upper, _proc->version );

	if( chdir( _proc->basedir ) )
		fatal( "Could not chdir to base dir %s -- %s", _proc->basedir, Err );
	else
		debug( "Working directory now %s", getcwd( NULL, 0 ) );

	if( runf_has( RUN_DAEMON ) )
	{
		if( daemon( 1, 0 ) < 0 )
		{
			fprintf( stderr, "Unable to daemonize -- %s", Err );
			warn( "Unable to daemonize -- %s", Err );
			_proc->run_flags &= ~RUN_DAEMON;
		}
		else
			info( "%s running in daemon mode, pid %d.", _proc->app_upper, getpid( ) );
	}

	if( writePid )
	{
		pidfile_write( );
		runf_add( RUN_PIDFILE );
	}

	// start up io
	io_init( );

	runf_add( RUN_APP_START );

	return 0;
}


void app_ready( void )
{
	double diff;

	if( !runf_has( RUN_APP_START ) )
		fatal( "App has not been started yet." );

	// run mem check / prealloc
	mem_startup( );

	get_time( );
	ts_diff( _proc->curr_time, _proc->init_time, &diff );

	info( "%s started up in %.3fs.", _proc->app_upper, diff );

	thread_throw( &loop_timer, NULL, 0 );

	runf_add( RUN_APP_READY );
	runf_add( RUN_LOOP );
}



void app_finish( int exval )
{
	int i;

	if( RUNNING( ) )
		loop_end( "app shutdown" );

	info( "Shutting down after %.3fs", get_uptime( ) );
	info( "Waiting for all threads to stop." );

	for( i = 0; i < 300 && _proc->loop_count > 0; i++ )
	{
		usleep( 100000 );
		if( !( i & 0x1f ) )
			debug( "Waiting..." );
	}

	if( _proc->loop_count <= 0 )
		info( "All threads have completed." );
	else
		notice( "Shutting down without thread completion." );

	io_stop( );

	pthread_mutex_destroy( &(_proc->loop_lock) );
	mem_shutdown( );

	if( runf_has( RUN_PIDFILE ) )
		pidfile_remove( );

	notice( "App %s v%s exiting.", _proc->app_name, _proc->version );
	log_close( );
	exit( exval );
}
