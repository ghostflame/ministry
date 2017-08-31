/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* main.c - program entry and startup/shutdown                             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry_test.h"

void usage( void )
{
	char *help_str = "\
Usage\tministry_test -h\n\
\tministry_test [OPTIONS] [-c <config file>]\n\n\
Options:\n\
 -h           Print this help\n\
 -c <path>    Select config file or URI to use\n\
 -E           Disable reading environment variables\n\
 -F           Disable reading a config file (env only)\n\
 -U           Disable all reading of URI's\n\
 -u           Disable URI config including other URI's\n\
 -i           Allow insecure URI's\n\
 -I           Allow secure URI's to include insecure URI's.\n\
 -D           Switch on debug output (overrides config)\n\
 -V           Verbose logging to console\n\
 -v           Print version number and exit\n\
 -s           Strict config parsing\n\
 -t           Just test the config is valid and exit\n\n\
Ministry_test is a load-generator for ministry.  It takes config specifying\n\
metric synthesis profiles, and profile generators with a prefix for metrics.\n\
It can be configured to point at either ministry or compat format targets.\n\n";

	printf( "%s", help_str );
	exit( 0 );
}


// these are used by config.c/conf_read
CSECT config_sections[CONF_SECT_MAX] =
{
	{ "main",    &config_line,        CONF_SECT_MAIN   },
	{ "logging", &log_config_line,    CONF_SECT_LOG    },
	{ "memory",  &mem_config_line,    CONF_SECT_MEM    },
	{ "metric",  &metric_config_line, CONF_SECT_METRIC },
	{ "target",  &target_config_line, CONF_SECT_TARGET }
};



void shut_down( int exval )
{
	int i;

	info( "Ministry_test shutting down after %.3fs", get_uptime( ) );

	info( "Waiting for all threads to stop." );

	for( i = 0; i < 300 && ctl->proc->loop_count > 0; i++ )
	{
		usleep( 100000 );
		if( !( i & 0x1f ) )
			debug( "Waiting..." );
	}

	if( ctl->proc->loop_count <= 0 )
		info( "All threads have completed." );
	else
		warn( "Shutting down without thread completion." );

	// shut down all those mutex locks
	lock_shutdown( );

	notice( "Ministry_test v%s exiting.", ctl->proc->version );
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
		if( ctl->proc->setlim[i] )
			ret += setlimit( i, ctl->proc->limits[i] );

	return ret;
}



void main_create_conf( void )
{
	ctl             = (MTEST_CTL *) allocz( sizeof( MTEST_CTL ) );

	ctl->proc       = config_defaults( );
	ctl->locks      = lock_config_defaults( );
	ctl->log        = log_config_defaults( );
	ctl->mem        = mem_config_defaults( );
	ctl->metric     = metric_config_defaults( );
	ctl->tgt        = target_config_defaults( );
}


void main_loop( void )
{
	// set us going
	ctl->proc->run_flags |= RUN_LOOP;

	get_time( );

	// start a timing circuit
	thread_throw( &loop_timer, NULL, 0 );

	// throw the memory loops
	thread_throw( &mem_check_loop, NULL, 0 );
	thread_throw( &mem_prealloc_loop, NULL, 0 );

	// init our targets
	target_init( );

	// and start the metrics (which start the targets)
	metric_start_all( );

	// and now we wait for the signal to end
	while( ctl->proc->run_flags & RUN_LOOP )
		sleep( 1 );
}



int main( int ac, char **av, char **env )
{
	int oc, debug = 0;
	double diff;

	// make a control structure
	main_create_conf( );

	while( ( oc = getopt( ac, av, "hHDVEFUIsuivtc:C:" ) ) != -1 )
		switch( oc )
		{
			case 'C':
				ctl->proc->strict = -1;
                config_set_main_file( optarg );
                break;
			case 'c':
                config_set_main_file( optarg );
				break;
			case 'v':
				printf( "Ministry_test version: %s\n", ctl->proc->version );
				return 0;
			case 'D':
				debug = 1;
				ctl->log->level = LOG_LEVEL_DEBUG;
				ctl->proc->run_flags |= RUN_DEBUG;
				break;
			case 'V':
				ctl->log->force_stdout = 1;
				break;
			case 't':
				setcfFlag( TEST_ONLY );
				break;
			case 's':
				ctl->proc->strict = -1;
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
	if( config_read( ctl->proc->cfg_file, NULL ) )
	{
		printf( "Config file '%s' is invalid.\n", ctl->proc->cfg_file );
		return 1;
	}

	// tidy up curl
	curl_global_cleanup( );

	// enforce what we got in arguments
	if( debug )
	{
		ctl->log->level = LOG_LEVEL_DEBUG;
		ctl->proc->run_flags |= RUN_DEBUG;
	}

	if( chkcfFlag( TEST_ONLY ) )
	{
		printf( "Config file '%s' is OK.\n", ctl->proc->cfg_file );
		return 0;
	}

	if( !ctl->log->force_stdout && !ctl->log->use_std )
		debug( "Starting logging - no more logs to stdout." );

	log_start( );
	notice( "Ministry_test v%s starting up.", ctl->proc->version );

	if( chdir( ctl->proc->basedir ) )
		fatal( "Could not chdir to base dir %s -- %s", ctl->proc->basedir, Err );
	else
		debug( "Working directory now %s", getcwd( NULL, 0 ) );

	if( set_limits( ) )
		fatal( "Failed to set limits." );

	if( set_signals( ) )
		fatal( "Failed to set signalling." );

	get_time( );
	ts_diff( ctl->proc->curr_time, ctl->proc->init_time, &diff );
	info( "Ministry_test started up in %.3fs.", diff );

	// only exits on a signal that removes RUN_LOOP
	main_loop( );

	// never returns
	shut_down( 0 );

	// this never happens but it keeps gcc happy
	return 0;
}


