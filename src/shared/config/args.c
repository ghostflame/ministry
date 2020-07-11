/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* config.c - read config files and create config object                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#include "local.h"


void config_help( void )
{
	printf(
"Usage\t%s --help | -h\n\
\t%s [OPTIONS] -c <config file>\n\n\
Options:\n", _proc->app_name, _proc->app_name );

	printf( "%s",
"\
  -h --help                   Print this help\n\
  -v --version                Print version number and exit\n\
  -t --config-test            Just test the config is valid and exit\n\
  -s --strict                 Strict config parsing\n\
  -c --config        <path>   Select config file or URI to use\n\
  -P --env-prefix  <prefix>   Set environment variable prefix\n\
  -E --no-environment         Disable reading environment variables\n\
  -F --no-config              Disable reading a config file (env only)\n\
  -R --no-include             Disable all including of other config\n\
  -U --no-uri                 Disable all reading of URI's\n\
  -u --no-uri-include         Disable URI config including other URI's\n\
  -X --no-suffix              Disable matching file suffix in include-dir\n\
  -x --suffix      <suffix>   Set config file suffix for include-dir\n\
  -i --insecure-uri           Allow insecure URI's\n\
  -I --insecure-include       Allow secure URI's to include insecure URI's\n\
  -K --interactive-pass       Interactively ask for an SSL key password\n\
  -T --validate-certs         Validate certificates for HTTPS (if available)\n\
  -W --invalid-fetch          Permit invalid certificates from fetch targets\n\
  -d --daemon                 Daemonize in the background\n\
  -D --debug                  Switch on debug output (overrides config)\n\
  -V --verbose                Logging to console (prevents daemonizing)\n\
" );
}


const char *config_args_opt_string = "HhDVvtsUuiIEKTWFdXx:P:c:";
char config_args_opt_merged[CONF_LINE_MAX];

struct option long_options[] = {
	// behaviours
	{ "help",               no_argument,          NULL, 'h' },
	{ "version",            no_argument,          NULL, 'w' },
	{ "config-test",        no_argument,          NULL, 't' },
	{ "daemon",             no_argument,          NULL, 'd' },
	// config
	{ "config",             required_argument,    NULL, 'c' },
	{ "env-prefix",         required_argument,    NULL, 'P' },
	{ "pidfile",            required_argument,    NULL, 'p' },
	{ "strict",             no_argument,          NULL, 's' },
	{ "suffix",             required_argument,    NULL, 'x' },
	// switches
	{ "no-environment",     no_argument,          NULL, 'E' },
	{ "no-config",          no_argument,          NULL, 'F' },
	{ "no-uri",             no_argument,          NULL, 'U' },
	{ "no-uri-include",     no_argument,          NULL, 'u' },
	{ "no-suffix",          no_argument,          NULL, 'X' },
	{ "no-byhand",          no_argument,          NULL, 'a' },
	// security
	{ "no-include",         no_argument,          NULL, 'R' },
	{ "insecure-uri",       no_argument,          NULL, 'i' },
	{ "insecure-include",   no_argument,          NULL, 'I' },
	{ "interactive-pass",   no_argument,          NULL, 'K' },
	{ "validate-certs",     no_argument,          NULL, 'T' },
	{ "invalid-fetch",      no_argument,          NULL, 'W' },
	// logging
	{ "verbose",            no_argument,          NULL, 'V' },
	{ "debug",              no_argument,          NULL, 'D' },
	// termination
	{ NULL,                 0,                    NULL,  0  }
};


char *config_arg_string( const char *argstr )
{
	snprintf( config_args_opt_merged, CONF_LINE_MAX, "%s%s", config_args_opt_string, argstr );
	return config_args_opt_merged;
}


void config_set_main_file( const char *path )
{
	if( _proc->cfg_file )
		free( _proc->cfg_file );

	_proc->cfg_file = str_copy( path, 0 );

	// this wins against env
	setcfFlag( FILE_OPT );
}

void config_set_env_prefix( const char *prefix )
{
	char prev = ' ';
	const char *p;
	int i;

	// it needs to be uppercase
	for( i = 0, p = prefix; i < 125 && *p; ++p, ++i )
	{
		if( *p == '-' )
			_proc->env_prfx[i] = '_';
		else
			_proc->env_prfx[i] = toupper( *p );

		prev = *p;
	}

	// and end with _
	if( prev != '_' )
		_proc->env_prfx[i++] = '_';

	_proc->env_prfx[i]  = '\0';
	_proc->env_prfx_len = i;
}

void config_set_suffix( const char *suffix )
{
	char tmp[256];
	int len;

	if( !suffix )
		suffix = "";

	if( *suffix == '.' )
		++suffix;

	if( *suffix )
		len = snprintf( tmp, 256, ".%s", suffix );
	else
		len = 0;

	if( _proc->conf_sfx )
	{
		free( _proc->conf_sfx );
		_proc->conf_sfx = NULL;
	}

	if( len )
		_proc->conf_sfx = str_copy( tmp, len );

	_proc->cfg_sffx_len = len;
}


void config_args( int ac, char **av, const char *optstr, help_fn *hfp )
{
	int oc, idx;

	while( ( oc = getopt_long( ac, av, optstr, long_options, &idx ) ) != -1 )
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
#ifdef DEBUG_MALLOC
				// set malloc debugging
				mallopt(M_CHECK_ACTION, 0x3);
#endif
				break;
			case 'V':
				runf_add( RUN_TGT_STDOUT );
				log_set_force_stdout( 1 );
				break;
			case 'x':
				config_set_suffix( optarg );
				break;
			case 'X':
				cutcfFlag( SUFFIX );
				break;
			case 'w':
				printf( "%s version: %s\n", _proc->app_upper, _proc->version );
				exit( 0 );
				break;
			case 'v':
				printf( "%s\n", _proc->version );
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
			case 'R':
				cutcfFlag( READ_INCLUDE );
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
			default:
				exit( 1 );
				break;
		}

	// reset arg processing
	optind = 1;
}



