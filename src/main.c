#include "ministry.h"

void usage( void )
{
	char *help_str = "\
Usage\tministry -h\n\
\tministry [OPTIONS] -c <config file>\n\n\
Options:\n\
 -h           Print this help\n\
 -c <file>    Select config file to use\n\
 -d           Daemonize in the background\n\
 -D           Switch on debug output (overrides config)\n\
 -v           Verbose logging to console\n\
 -t           Just test the config is valid and exit\n\n\
Ministry is a statsd-alternative processing engine.  It runs on very\n\
similiar lines, taking data paths and producing statistics on them.\n\
It submits data using a graphite format.\n\n";

	printf( "%s", help_str );
	exit( 0 );
}


void shut_down( int exval )
{
	double diff;
	int i;

	get_time( );
	diff = (double) tvdiff( ctl->curr_time, ctl->init_time );
	info( "Ministry shutting down after %.3fs", diff / 1000000.0 );

	// shut down ports
	net_stop( );

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

	// clean up the pidfile
	pidfile_remove( );

	notice( "Ministry exiting." );
	log_close( );
	exit( exval );
}



int set_signals( void )
{
	struct sigaction sa;

	memset( &sa, 0, sizeof( struct sigaction ) );
	sa.sa_handler = loop_kill;
	sa.sa_flags   = SA_NOMASK;

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

	return 0;
}



int main( int ac, char **av )
{
	int oc, justTest = 0;
	double diff;

	// make a control structure
	ctl = config_create( );

	while( ( oc = getopt( ac, av, "hHDdvtc:" ) ) != -1 )
		switch( oc )
		{
			case 'c':
				info( "Config file %s", optarg );
				ctl->cfg_file = strdup( optarg );
				break;
			case 'd':
				info( "Daemon mode." );
				ctl->run_flags |= RUN_DAEMON;
				break;
			case 'D':
				info( "Debug!" );
				ctl->log->level = LOG_LEVEL_DEBUG;
				ctl->run_flags  = RUN_DEBUG;
				break;
			case 'v':
				info( "Stdout logging." );
				ctl->log->force_stdout = 1;
				break;
			case 't':
				justTest = 1;
				break;
			case 'H':
			case 'h':
				usage( );
				break;
			default:
				err( "Unrecognised option %c", (char) oc );
				return 1;
		}

	// try to read the config
	if( config_read( ctl->cfg_file ) )
		fatal( "Unable to read config file '%s'", ctl->cfg_file );

	if( justTest )
	{
		notice( "Config is OK." );
		return 0;
	}

	if( chdir( ctl->basedir ) )
		fatal( "Could not chdir to base dir %s -- %s", ctl->basedir, Err );
	else
		debug( "Working directory now %s", getcwd( NULL, 0 ) );

	if( ctl->run_flags & RUN_DAEMON )
	{
		if( daemon( 1, 0 ) < 0 )
		{
			fprintf( stderr, "Unable to daemonize -- %s", Err );
			warn( "Unable to daemonize -- %s", Err );
			ctl->run_flags &= ~RUN_DAEMON;
		}
		else
			info( "Ministry running in daemon mode, pid %d.", getpid( ) );
	}

	if( set_signals( ) )
		fatal( "Failed to set signalling." );

	// create the data hash structures
	data_init( );

	if( net_start( ) )
		fatal( "Failed to start networking." );

	get_time( );
	diff = (double) tvdiff( ctl->curr_time, ctl->init_time );
	info( "Ministry started up in %.3fs.", diff / 1000000.0 );

	// only exits on a signal that removes RUN_LOOP
	loop_start( );

	// never returns
	shut_down( 0 );

	// this never happens but it keeps gcc happy
	return 0;
}


