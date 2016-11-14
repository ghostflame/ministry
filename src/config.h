/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* config.h - defines main config structure                                * 
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef MINISTRY_CONFIG_H
#define MINISTRY_CONFIG_H

#define DEFAULT_BASE_DIR		"/etc/ministry"
#define DEFAULT_CONFIG_FILE		"/etc/ministry/ministry.conf"


// config flags
#define CONF_READ_ENV			0x0001
#define CONF_READ_FILE			0x0002
#define CONF_READ_URL			0x0004
#define CONF_URL_INC_URL		0x0008
#define CONF_SEC_INC_UNSEC		0x0010


// used all over - so all the config line fns have an AVP called 'av'
#define attIs( s )      !strcasecmp( av->att, s )
#define valIs( s )      !strcasecmp( av->val, s )


struct config_context
{
  	CCTXT				*	next;
	CCTXT				*	children;
	CCTXT				*	parent;

	char					file[512];
	char					section[512];
	int						lineno;
	int						is_url;
	int						is_ssl;
};


// main control structure
struct ministry_control
{
	HTTP_CTL			*	http;
	LOG_CTL				*	log;
	LOCK_CTL			*	locks;
	MEM_CTL				*	mem;
	NET_CTL				*	net;
	STAT_CTL			*	stats;
	SYN_CTL				*	synth;
	TGT_CTL				*	tgt;

	struct timespec			init_time;
	struct timespec			curr_time;

	unsigned int			conf_flags;
	unsigned int			run_flags;

	char				*	cfg_file;
	char				*	pidfile;
	char				*	version;
	char				*	basedir;

	int						tick_usec;
	int						loop_count;

	int64_t					limits[RLIMIT_NLIMITS];
	int8_t					setlim[RLIMIT_NLIMITS];
};


int config_bool( AVP *av );
int config_read( char *path );
int config_read_env( char **env );
char *config_relative_path( char *inpath );
int config_choose_handler( char *section, AVP *av );
void config_create( void );

#endif
