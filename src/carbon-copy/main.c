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

	// begin sending our own stats
	self_stats_init( );

	while( RUNNING( ) )
		sleep( 1 );

	// shut down ports
	net_stop( );
	io_stop( );
}

RCTL *ctl = NULL;

void main_create_conf( void )
{
	ctl				= (RCTL *) mem_perm( sizeof( RCTL ) );
	ctl->proc		= app_control( );
	ctl->mem		= memt_config_defaults( );
	ctl->relay		= relay_config_defaults( );
	ctl->stats		= self_stats_config_defaults( );

	config_register_section( "relay", &relay_config_line );
	config_register_section( "stats", &self_stats_config_line );

	net_host_callbacks( &relay_buf_set, &relay_buf_end );
}



int main( int ac, char **av, const char **env )
{
	char *pidfile = NULL, *optstr;
	int oc;

	// start us up
	app_init( "carbon-copy", "ministry", RUN_NO_RKV );

	// set out default HTTP ports
	http_set_default_ports( CC_DEFAULT_HTTP_PORT, CC_DEFAULT_HTTPS_PORT );

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

	mem_set_max_kb( DEFAULT_CC_MAX_KB );

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
	if( relay_init( ) )
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


