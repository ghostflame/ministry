/**************************************************************************
* Copyright 2015 John Denholm                                             *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
*                                                                         *
*                                                                         *
* main.c - program entry and startup/shutdown                             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "min-monitor.h"

void usage( void )
{
	config_help( );
	printf( "%s", "\
  -p --pidfile       <file>   Override configured pidfile\n\n\
Monitor is a simple monitoring application designed to config that other\n\
applications are up.  It takes simple config for HTTP(S) monitoring and\n\
TCP monitoring, reporting results if desired.\n\n" );

	exit( 0 );
}


void main_loop( void )
{
	while( RUNNING( ) )
		sleep( 1 );

	// shut down ports
	net_stop( );
	io_stop( );
}

MOCTL *ctl = NULL;

void main_create_conf( void )
{
	ctl				= (MOCTL *) mem_perm( sizeof( MOCTL ) );
	ctl->proc		= app_control( );
	ctl->mem        = memt_config_defaults( );
	ctl->mons       = monitors_config_defaults( );

	config_register_section( "monitors", &monitors_config_line );
}



int main( int ac, char **av, const char **env )
{
	char *pidfile = NULL, *optstr;
	int oc;

	// start us up
	app_init( "min-monitor", "ministry", RUN_NO_RKV );

	// set our default HTTP ports
	http_set_default_ports( MON_DEFAULT_HTTP_PORT, MON_DEFAULT_HTTPS_PORT );

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

	mem_set_max_kb( DEFAULT_MON_MAX_KB );

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
	if( monitors_init( ) != 0 )
		fatal( "Unable to parse all monitor config." );

	// say we are ready
	app_ready( );

	// only exits on a signal that removes RUN_LOOP
	main_loop( );

	// never returns
	app_finish( 0 );

	// this never happens but it keeps gcc happy
	return 0;
}

