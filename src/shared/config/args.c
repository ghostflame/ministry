/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* config.c - read config files and create config object                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#include "local.h"


static char config_help_buffer[4096];

char *config_help( void )
{
	snprintf( config_help_buffer, 4096, "%s",
"\
 -h            Print this help\n\
 -v            Print version number and exit\n\
 -t            Just test the config is valid and exit\n\
 -c <path>     Select config file or URI to use\n\
 -C <path>     As -c but with strict processing\n\
 -E            Disable reading environment variables\n\
 -P <prefix>   Set environment variable prefix\n\
 -F            Disable reading a config file (env only)\n\
 -U            Disable all reading of URI's\n\
 -u            Disable URI config including other URI's\n\
 -i            Allow insecure URI's\n\
 -I            Allow secure URI's to include insecure URI's\n\
 -K            Interactively ask for an SSL key password\n\
 -T            Validate certificates for HTTPS (if available)\n\
 -W            Permit invalid certificates from fetch targets\n\
 -d            Daemonize in the background\n\
 -D            Switch on debug output (overrides config)\n\
 -V            Logging to console (prevents daemonizing)\n\
 -s            Strict config parsing\n\
" );
	return config_help_buffer;
}


const char *config_args_opt_string = "HhDVvtsUuiIEKTWFdC:c:";
char config_args_opt_merged[CONF_LINE_MAX];

char *config_arg_string( char *argstr )
{
	snprintf( config_args_opt_merged, CONF_LINE_MAX, "%s%s", config_args_opt_string, argstr );
	return config_args_opt_merged;
}


void config_set_main_file( char *path )
{
	snprintf( _proc->cfg_file, CONF_LINE_MAX, "%s", path );

	// this wins against env
	setcfFlag( FILE_OPT );
}

void config_set_env_prefix( char *prefix )
{
	char *p, prev = ' ';
	int i;

	// it needs to be uppercase
	for( i = 0, p = prefix; i < 125 && *p; p++ )
	{
		_proc->env_prfx[i++] = toupper( *p );
		prev = *p;
	}

	// and end with _
	if( prev != '_' )
		_proc->env_prfx[i++] = '_';

	_proc->env_prfx[i]  = '\0';
	_proc->env_prfx_len = i;
}


void config_args( int ac, char **av, char *optstr, help_fn *hfp )
{
	int oc;

	while( ( oc = getopt( ac, av, optstr ) ) != -1 )
		switch( oc )
		{
			case 'H':
			case 'h':
				(*hfp)( );
				break;
			case 'C':
				_proc->strict = -1;
				config_set_main_file( optarg );
				break;
			case 'c':
				config_set_main_file( optarg );
				break;
			case 'd':
				runf_add( RUN_DAEMON );
				break;
			case 'D':
				log_set_level( LOG_LEVEL_DEBUG, 1 );
				runf_add( RUN_DEBUG );
				break;
			case 'V':
				runf_add( RUN_TGT_STDOUT );
				log_set_force_stdout( 1 );
				break;
			case 'v':
				printf( "%s version: %s\n", _proc->app_upper, _proc->version );
				exit( 0 );
				break;
			case 't':
				setcfFlag( TEST_ONLY );
				break;
			case 's':
				_proc->strict = -1;
				break;
			case 'P':
				config_set_env_prefix( optarg );
				break;
			case 'U':
				cutcfFlag( URL_INC_URL );
				cutcfFlag( READ_URL );
				break;
			case 'u':
				cutcfFlag( URL_INC_URL );
				break;
			case 'K':
				setcfFlag( KEY_PASSWORD );
				break;
			case 'i':
				setcfFlag( URL_INSEC );
				break;
			case 'I':
				setcfFlag( SEC_INC_INSEC );
				break;
#if _LCURL_CAN_VERIFY > 0
			case 'T':
				setcfFlag( SEC_VALIDATE );
				break;
			case 'W':
				setcfFlag( SEC_VALIDATE_F );
				break;
#endif
			case 'E':
				cutcfFlag( READ_ENV );
				break;
			case 'F':
				cutcfFlag( READ_FILE );
				break;
		}

	// reset arg processing
	optind = 1;
}



