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
	printf( "%s", "\
Usage\tministry -h\n\
\tministry [OPTIONS] [-c <config file>]\n\n\
Options:\n\
 -h            Print this help\n\
 -p <file>     Override configured pidfile\n\
 -v            Print version number and exit\n" );
	printf( "%s", config_help( ) );
	printf( "%s", "\n\
Ministry is a statsd-alternative processing engine.  It runs on very\n\
similiar lines, taking data paths and producing statistics on them.\n\
It submits data using a graphite format.\n\n" );

	exit( 0 );
}



void main_loop( void )
{
	// sort out the metrics attribute maps
	if( metrics_init( ) )
	{
		fatal( "Could not resolve all metric attribute config." );
		return;
	}

	// and the tset/target loops
	// must happen before stats_start
	targets_start( );

	// throw the data submission loops
	stats_start( ctl->stats->stats );
	stats_start( ctl->stats->adder );
	stats_start( ctl->stats->gauge );
	stats_start( ctl->stats->self );

	// and a synthetics loop
	thread_throw_named( &synth_loop, NULL, 0, "synth_loop" );

	// and gc
	thread_throw_named( &gc_loop, NULL, 0, "gc_loop" );

	// and token cleanup
	thread_throw_named( &token_loop, NULL, 0, "token_loop" );

	// throw the data listener loop
	net_start_type( ctl->net->stats );
	net_start_type( ctl->net->adder );
	net_start_type( ctl->net->gauge );
	net_start_type( ctl->net->compat );

	// and any fetch loops
	fetch_init( );

	// and wait
	while( RUNNING( ) )
		sleep( 1 );

	// shut down ports
	net_stop( );

	// and http server
	http_stop( );

	// unset some locks
	lock_shutdown( );
}



void main_create_conf( void )
{
	ctl             = (MIN_CTL *) allocz( sizeof( MIN_CTL ) );
	ctl->proc       = app_control( );
	ctl->mem        = memt_config_defaults( );
	ctl->gc         = gc_config_defaults( );
	ctl->locks      = lock_config_defaults( );
	ctl->net        = net_config_defaults( );
	ctl->stats      = stats_config_defaults( );
	ctl->synth      = synth_config_defaults( );
	ctl->tgt        = targets_config_defaults( );
	ctl->fetch      = fetch_config_defaults( );
	ctl->metric     = metrics_config_defaults( );

	config_register_section( "network", &net_config_line );
	config_register_section( "gc",      &gc_config_line );
	config_register_section( "stats",   &stats_config_line );
	config_register_section( "synth",   &synth_config_line );
	config_register_section( "fetch",   &fetch_config_line );
	config_register_section( "metrics", &metrics_config_line );

	target_set_type_fn( &targets_set_type );
}


int main( int ac, char **av, char **env )
{
	char *pidfile = NULL, *optstr;
	int oc;

	// start the app up
	app_init( "ministry", "ministry" );

	// make our combined arg string
	if( !( optstr = config_arg_string( "p:" ) ) )
		return 1;

	// make a control structure
	main_create_conf( );

	// let config have at the args
	config_args( ac, av, optstr, &usage );

	// and now our go
	while( ( oc = getopt( ac, av, optstr ) ) != -1 )
		switch( oc )
		{
			case 'p':
				pidfile = strdup( optarg );
				break;
		}

	// read our environment
	// has to happen after parsing args
	config_read_env( env );

	// try to read the config
	if( config_read( ctl->proc->cfg_file, NULL ) )
	{
		printf( "Config file '%s' is invalid.\n", ctl->proc->cfg_file );
		return 1;
	}

	if( pidfile )
		config_set_pid_file( pidfile );

	app_start( 1 );

	// set up targets data structures
	// must occur before stats init
	targets_init( );

	// set up stats and data structures
	stats_init( );

	// and any synethics
	synth_init( );

	// lights up networking and starts listening
	// also connects to graphite
	if( net_start( ) )
		fatal( "Failed to start networking." );

	// say we are up
	app_ready( );

	// only exits when killed
	main_loop( );

	// never returns
	app_finish( 0 );

	// this never happens but it keeps gcc happy
	return 0;
}


