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
};


struct config_writedata
{
	char				*	url;
	int						fd;
	int						ssl;
	int						err;
	int						bc;
};


// main control structure
struct ministry_control
{
	LOG_CTL				*	log;
	LOCK_CTL			*	locks;
	MEM_CTL				*	mem;
	NET_CTL				*	net;
	STAT_CTL			*	stats;
	SYN_CTL				*	synth;

	struct timespec			init_time;
	struct timespec			curr_time;

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
char *config_relative_path( char *inpath );
int config_choose_handler( char *section, AVP *av );
void config_create( char **env );

#endif
