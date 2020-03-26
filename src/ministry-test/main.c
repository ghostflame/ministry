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
	config_help( );
	printf( "%s", "\
  -a --no-byhand              Re-enable automation features normally disabled\n\n\
Ministry_test is a load-generator for ministry.  It takes config specifying\n\
metric synthesis profiles, and profile generators with a prefix for metrics.\n\
It can be configured to point at either ministry or compat format targets.\n\n" );

	exit( 0 );
}



void main_create_conf( void )
{
	ctl             = (MTEST_CTL *) mem_perm( sizeof( MTEST_CTL ) );

	ctl->proc       = app_control( );
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



int main( int ac, char **av, const char **env )
{
	char *optstr;
	int oc;

	// this first
	app_init( "ministry-test", "ministry", RUN_NO_HTTP|RUN_NO_RKV );

	// say we are a by-hand app
	runf_add( RUN_BY_HAND );

	if( !( optstr = config_arg_string( "a" ) ) )
		return 1;

	// make a control structure
	main_create_conf( );

	// let config have the args
	config_args( ac, av, optstr, &usage );

	while( ( oc = getopt( ac, av, optstr ) ) != -1 )
		switch( oc )
		{
			case 'a':
				runf_rmv( RUN_BY_HAND );
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

	mem_set_max_kb( DEFAULT_MT_MAX_KB );
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


