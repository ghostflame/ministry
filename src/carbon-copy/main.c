/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* main.c - program entry and startup/shutdown                             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "carbon_copy.h"

void usage( void )
{
	config_help( );
	printf( "%s", "\
  -p --pidfile       <file>   Override configured pidfile\n\n\
Carbon-copy is a carbon-relay alternative data router.  It runs on very similar\n\
lines, matching lines against patterns or rules and forwarding them to coal\n\
or carbon-cache (or other copies of carbon-copy).\n\n" );

	exit( 0 );
}


void main_loop( void )
{
	// and the network io loops
	target_run( );

	// throw the data listener loop
	net_begin( );

	while( RUNNING( ) )
		sleep( 1 );

	// shut down ports
	net_stop( );
	io_stop( );
}


void main_create_conf( void )
{
	ctl				= (RCTL *) allocz( sizeof( RCTL ) );
	ctl->proc		= app_control( );
	ctl->mem		= memt_config_defaults( );
	ctl->net		= network_config_defaults( );
	ctl->relay		= relay_config_defaults( );

	config_register_section( "relay", &relay_config_line );

	net_host_callbacks( &relay_buf_set, &relay_buf_end );
}



int main( int ac, char **av, char **env )
{
	char *pidfile = NULL, *optstr;
	int oc;

	// start us up
	app_init( "carbon-copy", "ministry" );

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

	mem_set_max_kb( DEFAULT_CC_MAX_KB );
	app_start( 1 );

	// resolve relay targets
	if( relay_resolve( ) )
		fatal( "Unable to resolve all relay targets." );

	// lights up networking and starts listening
	// also connects to graphite
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


