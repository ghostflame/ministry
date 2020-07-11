/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* main.c - program entry and startup/shutdown                             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "metric_filter.h"

void usage( void )
{
	config_help( );
	printf( "%s", "\
  -p --pidfile       <file>   Override configured pidfile\n\n\
Metric-filter is a simple metrics filtering tool which does line-by-line\n\
filtering.  It accepts config linking hosts to regular expressions and then\n\
matches the data received from them, dropping or forwarding as desired.\n\n" );

	exit( 0 );
}


void main_loop( void )
{
	// and the network io loops
	target_run( );

	// throw the data listener loop
	net_begin( );

	// begin sending our own stats
	//self_stats_init( );

	while( RUNNING( ) )
		sleep( 1 );

	// shut down ports
	net_stop( );
	io_stop( );
}


void main_create_conf( void )
{
	ctl				= (MCTL *) mem_perm( sizeof( MCTL ) );
	ctl->proc		= app_control( );
	ctl->mem		= memt_config_defaults( );
	ctl->filt       = filter_config_defaults( );
	ctl->netw       = network_config_defaults( );

	config_register_section( "filter", &filter_config_line );

	net_host_callbacks( &filter_host_setup, &filter_host_end );
}



int main( int ac, char **av, const char **env )
{
	char *pidfile = NULL, *optstr;
	int oc;

	// start us up
	app_init( "metric-filter", "ministry", RUN_NO_RKV );

	// set our default HTTP ports
	http_set_default_ports( MF_DEFAULT_HTTP_PORT, MF_DEFAULT_HTTPS_PORT );

	if( !( optstr = config_arg_string( "p:" ) ) )
		return 1;

	// create ctl
	main_create_conf( );

	// let config have at the args
	config_args( ac, av, optstr, &usage );

	while( ( oc = getopt( ac, av, optstr ) ) != -1 )
		switch( oc )
		{
			case 'p':
				pidfile = strdup( optarg );
				break;
		}

	mem_set_max_kb( DEFAULT_MF_MAX_KB );

	// read our env; has to happen before parsing config
	// because we might get handed a config file in env
	config_read_env( env );

	// try to read the config
	if( config_read( ctl->proc->cfg_file, NULL ) )
	{
		printf( "Config file '%s' is invalid.\n", ctl->proc->cfg_file );
		return 1;
	}

	if( pidfile )
		snprintf( ctl->proc->pidfile, CONF_LINE_MAX, "%s", pidfile );

	app_start( 1 );

	// resolve relay targets
	if( filter_init( ) != 0 )
		fatal( "Unable to parse all filter config." );

	// lights up networking and starts listening
	// also connects to our target
	if( net_start( ) )
		fatal( "Failed to start networking." );

	// say we are ready
	app_ready( );

	// only exits on a signal that removes RUN_LOOP
	main_loop( );

	// never returns
	app_finish( 0 );

	// this never happens but it keeps gcc happy
	return 0;
}

