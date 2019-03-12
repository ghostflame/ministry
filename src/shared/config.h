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

#define CONF_SECT_MAX				128
#define CONF_LINE_MAX				8192

// config flags
#define CONF_FLAG_READ_ENV			0x00000001
#define CONF_FLAG_READ_FILE			0x00000002
#define CONF_FLAG_READ_URL			0x00000004
#define CONF_FLAG_URL_INSEC			0x00000100
#define CONF_FLAG_URL_INC_URL		0x00000200
#define CONF_FLAG_SEC_INC_INSEC		0x00000400
#define CONF_FLAG_SEC_VALIDATE		0x00001000
#define CONF_FLAG_SEC_VALIDATE_F	0x00002000
#define CONF_FLAG_KEY_PASSWORD		0x00004000
#define CONF_FLAG_TEST_ONLY			0x10000000
#define CONF_FLAG_FILE_OPT			0x20000000

#define XsetcfFlag( p, K )			(p)->conf_flags |= CONF_FLAG_##K
#define XcutcfFlag( p, K )			(p)->conf_flags &= ~(CONF_FLAG_##K)
#define XchkcfFlag( p, K )			( (p)->conf_flags & CONF_FLAG_##K )

#define setcfFlag( K )				XsetcfFlag( _proc, K )
#define cutcfFlag( K )				XcutcfFlag( _proc, K )
#define chkcfFlag( K )				XchkcfFlag( _proc, K )


// used all over - so all the config line fns have an AVP called 'av'
#define attIs( s )      !strcasecmp( av->aptr, s )
#define valIs( s )      !strcasecmp( av->vptr, s )


struct config_section
{
	char				*	name;
	conf_line_fn		*	fp;
	int						section;
};

// bring this in from main
extern CSECT config_sections[];

struct config_context
{
	CCTXT				*	next;
	CCTXT				*	children;
	CCTXT				*	parent;
	CSECT				*	section;

	char					source[4096];
	char				**	argv;
	int						lineno;
	int						is_url;
	int						is_ssl;
	int					*	argl;
	int						argc;
};



// main control structure
struct process_control
{
	struct timespec			init_time;
	struct timespec			curr_time;
	int64_t					curr_tval;

	unsigned int			conf_flags;
	unsigned int			run_flags;

	char				*	app_name;
	char					app_upper[CONF_LINE_MAX];
	char				*	version;
	char					env_prfx[128];
	char					cfg_file[CONF_LINE_MAX];
	char					pidfile[CONF_LINE_MAX];
	char					basedir[CONF_LINE_MAX];

	int64_t					tick_usec;
	int64_t					loop_count;

	pthread_mutex_t			loop_lock;
	pthread_mutexattr_t		mtxa;

	int						env_prfx_len;
	int						strict;
	int						sect_count;

	int64_t					limits[RLIMIT_NLIMITS];
	int8_t					setlim[RLIMIT_NLIMITS];

	// string stores
	SSTR				*	stores;

	// the other pieces
	LOG_CTL				*	log;
	MEM_CTL				*	mem;
	HTTP_CTL			*	http;
	IO_CTL				*	io;
	IPL_CTL				*	ipl;
	TGT_CTL				*	tgt;
};




conf_line_fn config_line;

char *config_help( void );

char *config_arg_string( char *argstr );
void config_args( int ac, char **av, char *optstr, help_fn *hfp );

int config_bool( AVP *av );
int config_read( char *path, WORDS *w );
int config_read_env( char **env );
char *config_relative_path( char *inpath );
void config_choose_section( CCTXT *c, char *section );

void config_register_section( char *name, conf_line_fn *fp );

void config_set_main_file( char *path );
void config_set_env_prefix( char *prefix );
void config_set_pid_file( char *path );

PROC_CTL *config_defaults( char *app_name, char *conf_dir );

#endif
