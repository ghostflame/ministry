/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* config.h - defines main config structure                                * 
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef SHARED_CONFIG_H
#define SHARED_CONFIG_H

#define CONF_LINE_MAX				8192

// config flags
#define CONF_FLAG_READ_ENV			0x00000001
#define CONF_FLAG_READ_FILE			0x00000002
#define CONF_FLAG_READ_URL			0x00000004
#define CONF_FLAG_READ_INCLUDE      0x00000008
#define CONF_FLAG_URL_INSEC			0x00000100
#define CONF_FLAG_URL_INC_URL		0x00000200
#define CONF_FLAG_SEC_INC_INSEC		0x00000400
#define CONF_FLAG_SEC_VALIDATE		0x00001000
#define CONF_FLAG_SEC_VALIDATE_F	0x00002000
#define CONF_FLAG_KEY_PASSWORD		0x00004000
#define CONF_FLAG_SUFFIX			0x00010000
#define CONF_FLAG_CHG_EXIT			0x00020000
#define CONF_FLAG_TEST_ONLY			0x10000000
#define CONF_FLAG_FILE_OPT			0x20000000

#define XsetcfFlag( p, K )			(p)->conf_flags |= CONF_FLAG_##K
#define XcutcfFlag( p, K )			(p)->conf_flags &= ~(CONF_FLAG_##K)
#define XchkcfFlag( p, K )			( (p)->conf_flags & CONF_FLAG_##K )

#define setcfFlag( K )				XsetcfFlag( _proc, K )
#define cutcfFlag( K )				XcutcfFlag( _proc, K )
#define chkcfFlag( K )				XchkcfFlag( _proc, K )

#define TMP_DIR						"/tmp"
#define MAX_JSON_SZ					0x8000		// 32k

// used all over - so all the config line fns have an AVP called 'av'
#define attIs( s )					!strcasecmp( av->aptr, s )
#define attIsN( s, n )				!strncasecmp( av->aptr, s, n )
#define valIs( s )					!strcasecmp( av->vptr, s )


#define config_lock( )				pthread_mutex_lock(   &(_proc->cfg_lock) )
#define config_unlock( )			pthread_mutex_unlock( &(_proc->cfg_lock) )



// main control structure
struct process_control
{
	struct timespec			init_time;
	struct timespec			curr_time;
	int64_t					curr_tval;
	int64_t					curr_usec;

	unsigned int			conf_flags;
	unsigned int			run_flags;

	char				*	hostname;
	char				*	app_name;
	char				*	app_upper;
	char				*	version;
	char				*	tmpdir;
	char				*	conf_sfx;
	char					env_prfx[128];
	char					cfg_file[CONF_LINE_MAX];
	char					pidfile[CONF_LINE_MAX];
	char					basedir[CONF_LINE_MAX];

	//SLKHD				*	apphdl;

	int64_t					tick_usec;
	int64_t					loop_count;

	pthread_mutex_t			loop_lock;

	int						env_prfx_len;
	int						cfg_sffx_len;
	int						strict;
	int						sect_count;
	int						max_json_sz;

	int64_t					limits[RLIMIT_NLIMITS];
	int8_t					setlim[RLIMIT_NLIMITS];

	pthread_mutex_t			cfg_lock;

	// all file watch trees
	FTREE				*	watches;

	// file watch for config
	FTREE				*	cfiles;
	// has our config changed
	int64_t					cfgChanged;

	// the other pieces
	LOG_CTL				*	log;
	MEM_CTL				*	mem;
	HTTP_CTL			*	http;
	IO_CTL				*	io;
	IPL_CTL				*	ipl;
	TGT_CTL				*	tgt;
	HA_CTL				*	ha;
	PMET_CTL			*	pmet;
	NET_CTL				*	net;
	SLK_CTL				*	slk;
	STR_CTL				*	str;
};




conf_line_fn config_line;

loop_call_fn config_check_times;
throw_fn config_monitor;

ftree_callback config_on_change;

void config_help( void );

char *config_arg_string( const char *argstr );
void config_args( int ac, char **av, const char *optstr, help_fn *hfp );

int config_bool( AVP *av );
int config_read( const char *path, WORDS *w );
int config_read_env( const char **env );

void config_set_pid_file( const char *path );

// things that require the other structures
void config_late_setup( void );

void config_register_section( const char *name, conf_line_fn *fp );
PROC_CTL *config_defaults( const char *app_name, const char *conf_dir );

#endif
