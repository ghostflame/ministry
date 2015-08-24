#ifndef MINISTRY_CONFIG_H
#define MINISTRY_CONFIG_H

#define DEFAULT_BASE_DIR		"/etc/ministry"
#define DEFAULT_CONFIG_FILE		"ministry.conf"



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


// main control structure
struct ministry_control
{
	LOG_CTL				*	log;
	LOCK_CTL			*	locks;
	MEM_CTL				*	mem;
	NET_CTL				*	net;
	DATA_CTL			*	data;
	STAT_CTL			*	stats;

	struct timeval			init_time;
	struct timeval			curr_time;

	unsigned int			run_flags;

	char				*	cfg_file;
	char				*	pidfile;

	char				*	basedir;

	int						tick_usec;
	int						loop_count;
};



int config_read( char *path );
char *config_relative_path( char *inpath );
MIN_CTL *config_create( void );

#endif
