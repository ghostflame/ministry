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
	printf( "%s", "\
Usage\tministry_test -h\n\
\tministry_test [OPTIONS] [-c <config file>]\n\n\
Options:\n\
 -h            Print this help\n\
 -v            Print version number and exit\n" );
	printf( "%s", config_help( ) );
	printf( "%s", "\n\
Ministry_test is a load-generator for ministry.  It takes config specifying\n\
metric synthesis profiles, and profile generators with a prefix for metrics.\n\
It can be configured to point at either ministry or compat format targets.\n\n" );

	exit( 0 );
}



void main_create_conf( void )
{
	ctl             = (MTEST_CTL *) allocz( sizeof( MTEST_CTL ) );

	ctl->proc       = _proc;
	ctl->target     = _tgt;
	ctl->log        = _logger;
	ctl->metric     = metric_config_defaults( );
	ctl->tgt        = targets_config_defaults( );

	config_register_section( "metric",  &metric_config_line );

	target_set_type_fn( &targets_set_type );
}


void main_loop( void )
{
	// make sure we have suitable target types
	targets_resolve( );

	// and start the metrics (which start the targets)
	metric_start_all( );

	// and now we wait for the signal to end
	while( RUNNING( ) )
		sleep( 1 );
}



int main( int ac, char **av, char **env )
{
	char *optstr;
	//int oc;

	// this first
	app_init( "ministry_test" );

	if( !( optstr = config_arg_string( "" ) ) )
		return 1;

	// make a control structure
	main_create_conf( );

	// let config have the args
	config_args( ac, av, optstr, &usage );

	/* no local args
	while( ( oc = getopt( ac, av, optstr ) ) != -1 )
		switch( oc )
		{
		}
	*/

	// read our environment
	// has to happen after parsing args
	config_read_env( env );

	// try to read the config
	if( config_read( ctl->proc->cfg_file, NULL ) )
	{
		printf( "Config file '%s' is invalid.\n", ctl->proc->cfg_file );
		return 1;
	}

	app_start( 0 );

	// no extra startup

	app_ready( );

	// only exits on a signal that removes RUN_LOOP
	main_loop( );

	// never returns
	app_finish( 0 );

	// this never happens but it keeps gcc happy
	return 0;
}

