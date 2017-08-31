/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* main.c - program entry and startup/shutdown                             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"

void usage( void )
{
	char *help_str = "\
Usage\tministry -h\n\
\tministry [OPTIONS] [-c <config file>]\n\n\
Options:\n\
 -h           Print this help\n\
 -c <path>    Select config file or URI to use\n\
 -E           Disable reading environment variables\n\
 -F           Disable reading a config file (env only)\n\
 -U           Disable all reading of URI's\n\
 -u           Disable URI config including other URI's\n\
 -i           Allow insecure URI's\n\
 -I           Allow secure URI's to include insecure URI's.\n\
 -d           Daemonize in the background\n\
 -D           Switch on debug output (overrides config)\n\
 -V           Verbose logging to console\n\
 -p <file>    Override configured pidfile\n\
 -v           Print version number and exit\n\
 -s           Strict config parsing\n\
 -t           Just test the config is valid and exit\n\n\
Ministry is a statsd-alternative processing engine.  It runs on very\n\
similiar lines, taking data paths and producing statistics on them.\n\
It submits data using a graphite format.\n\n";

	printf( "%s", help_str );
	exit( 0 );
}


void shut_down( int exval )
{
	int i;

	info( "Ministry shutting down after %.3fs", get_uptime( ) );

	// shut down ports
	net_stop( );

	// and http server
	http_stop( );

	info( "Waiting for all threads to stop." );

	for( i = 0; i < 300 && ctl->loop_count > 0; i++ )
	{
		usleep( 100000 );
		if( !( i & 0x1f ) )
			debug( "Waiting..." );
	}

	if( ctl->loop_count <= 0 )
		info( "All threads have completed." );
	else
		warn( "Shutting down without thread completion." );

	// shut down all those mutex locks
	lock_shutdown( );

	pidfile_remove( );

	notice( "Ministry v%s exiting.", ctl->version );
	log_close( );
	exit( exval );
}


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
		if( ctl->setlim[i] )
			ret += setlimit( i, ctl->limits[i] );

	return ret;
}



int main( int ac, char **av, char **env )
{
	int oc, debug = 0;
	char *pidfile = NULL;
	double diff;

	// make a control structure
	config_create( );

	while( ( oc = getopt( ac, av, "hHDVEFUIsuidvtc:p:C:" ) ) != -1 )
		switch( oc )
		{
			case 'C':
				ctl->strict = -1;
				config_set_main_file( optarg );
				break;
			case 'c':
				config_set_main_file( optarg );
				break;
			case 'v':
				printf( "Ministry version: %s\n", ctl->version );
				return 0;
			case 'd':
				ctl->run_flags |= RUN_DAEMON;
				break;
			case 'p':
				pidfile = strdup( optarg );
				break;
			case 'D':
				debug = 1;
				ctl->log->level = LOG_LEVEL_DEBUG;
				ctl->run_flags |= RUN_DEBUG;
				break;
			case 'V':
				ctl->log->force_stdout = 1;
				break;
			case 't':
				setcfFlag( TEST_ONLY );
				break;
			case 's':
				ctl->strict = -1;
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
			case 'E':
				cutcfFlag( READ_ENV );
				break;
			case 'F':
				cutcfFlag( READ_FILE );
				break;
			case 'H':
			case 'h':
				usage( );
				break;
			default:
				err( "Unrecognised option %c", (char) oc );
				return 1;
		}

	// read our environment
	// has to happen after parsing args
	config_read_env( env );

	// set up curl
	curl_global_init( CURL_GLOBAL_SSL );

	// try to read the config
	if( config_read( ctl->cfg_file ) )
	{
		printf( "Config file '%s' is invalid.\n", ctl->cfg_file );
		return 1;
	}

	// tidy up curl
	curl_global_cleanup( );

	// enforce what we got in arguments
	if( debug )
	{
		ctl->log->level = LOG_LEVEL_DEBUG;
		ctl->run_flags |= RUN_DEBUG;
	}
	if( pidfile )
	{
		free( ctl->pidfile );
		ctl->pidfile = pidfile;
	}

	if( chkcfFlag( TEST_ONLY ) )
	{
		printf( "Config file '%s' is OK.\n", ctl->cfg_file );
		return 0;
	}

	if( !ctl->log->force_stdout && !ctl->log->use_std )
		debug( "Starting logging - no more logs to stdout." );

	log_start( );
	notice( "Ministry v%s starting up.", ctl->version );

	if( chdir( ctl->basedir ) )
		fatal( "Could not chdir to base dir %s -- %s", ctl->basedir, Err );
	else
		debug( "Working directory now %s", getcwd( NULL, 0 ) );

	if( ctl->run_flags & RUN_DAEMON )
	{
		if( ctl->run_flags & RUN_TGT_STDOUT )
		{
			warn( "Daemon mode disabled by stdout target." );
		}
		else if( daemon( 1, 0 ) < 0 )
		{
			fprintf( stderr, "Unable to daemonize -- %s", Err );
			warn( "Unable to daemonize -- %s", Err );
			ctl->run_flags &= ~RUN_DAEMON;
		}
		else
			info( "Ministry running in daemon mode, pid %d.", getpid( ) );
	}

	pidfile_write( );

	if( set_limits( ) )
		fatal( "Failed to set limits." );

	if( set_signals( ) )
		fatal( "Failed to set signalling." );

	// set up stats and data structures
	stats_init( );

	// and any synethics
	synth_init( );

	// lights up networking and starts listening
	// also connects to graphite
	if( net_start( ) )
		fatal( "Failed to start networking." );

	// light up http server if configured
	if( http_start( ) )
		fatal( "Failed to start HTTP server." );

	get_time( );
	ts_diff( ctl->curr_time, ctl->init_time, &diff );
	info( "Ministry started up in %.3fs.", diff );

	// only exits on a signal that removes RUN_LOOP
	loop_start( );

	// never returns
	shut_down( 0 );

	// this never happens but it keeps gcc happy
	return 0;
}


