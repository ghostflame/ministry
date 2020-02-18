/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* main.c - program entry and startup/shutdown                             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "archivist.h"

void usage( void )
{
	config_help( );
	printf( "%s", "\
  -p --pidfile       <file>   Override configured pidfile\n\n\
Archivist is a time-series database.  It combines both receipt and\n\
storage of points with a query engine.  It stores data in its own\n\
format where aggregation stores several metrics about the aggregated\n\
period.  It is expected to be both CPU, memory and IO heavy.\n\n" );

	exit( 0 );
}



void main_loop( void )
{
	tree_init( );
	query_init( );

	if( file_init( ) != 0 )
	{
		fatal( "Aborting - file storage path cannot be reached." );
		return;
	}

	// get network threads going
	net_begin( );

	// and wait
	while( RUNNING( ) )
		sleep( 1 );
}



void main_create_conf( void )
{
	ctl             = (RKV_CTL *) allocz( sizeof( RKV_CTL ) );
	ctl->proc       = app_control( );
	ctl->mem        = memt_config_defaults( );
	ctl->tree       = tree_config_defaults( );
	ctl->query      = query_config_defaults( );
	ctl->netw       = network_config_defaults( );
	ctl->file       = file_config_defaults( );

	config_register_section( "tree",    &tree_config_line );
	config_register_section( "query",   &query_config_line );
	config_register_section( "file",    &file_config_line );
}


int main( int ac, char **av, char **env )
{
	char *pidfile = NULL, *optstr;
	int oc;

	// start the app up
	app_init( "archivist", "ministry" );

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

	// set the max kb
	mem_set_max_kb( DEFAULT_RK_MAX_KB );

	// and turn on the webserver
	http_enable( );

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


