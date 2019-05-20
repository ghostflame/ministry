/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* ha.h - defines high-availability structures and functions               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#ifndef SHARED_HA_H
#define SHARED_HA_H

#define DEFAULT_HA_CHECK_MSEC		1000
#define DEFAULT_HA_UPDATE_MSEC		1000
#define DEFAULT_HA_CHECK_PATH		"/cluster"
#define DEFAULT_HA_CTL_PATH			"/control/cluster"



enum ha_elector_types
{
	HA_ELECT_RANDOM = 0,
	HA_ELECT_FIRST,
	HA_ELECT_MANUAL,
	HA_ELECT_MAX
};

enum ha_election_state_types
{
	HA_ELECTION_CONFIG = 0,
	HA_ELECTION_CHOOSING,
	HA_ELECTION_DONE,
	HA_ELECTION_MISMATCH,
	HA_ELECTION_MAX
};


enum ha_clst_line_kind_types
{
	HA_CLST_LINE_KIND_STATUS = 0,
	HA_CLST_LINE_KIND_SETTING,
	HA_CLST_LINE_KIND_PARTNER,
	HA_CLST_LINE_KIND_MAX
};

enum ha_clst_line_setting_types
{
	HA_CLST_LINE_SETT_ELECTOR = 0,
	HA_CLST_LINE_SETT_PERIOD,
	HA_CLST_LINE_SETT_UPDATE,
	HA_CLST_LINE_SETT_MAX
};



struct ha_partner
{
	HAPT			*	next;
	char			*	host;
	char			*	name;	// host:port

	char			*	check_url;
	char			*	ctl_url;
	CURLWH			*	ch;
	IOBUF			*	buf;

	uint16_t			port;
	int16_t				nlen;

	int64_t				tmout;
	int64_t				tmout_orig;
	int					use_tls;
	int					is_master;
	int					is_self;
};

#define lock_ha( )		pthread_mutex_lock(   &(_proc->ha->lock) )
#define unlock_ha( )	pthread_mutex_unlock( &(_proc->ha->lock) )

struct ha_control
{
	HAPT			*	partners;
	HAPT			*	self;
	int					pcount;

	pthread_mutex_t		lock;

	char			*	hostname;

	int64_t				period;		// msec
	int64_t				update; 	// msec

	int					priority;
	int					elector;	// style of election

	int					is_master;
	int					enabled;
};



const char *ha_elector_name( int elector );
const char *ha_election_state_name( int state );
int ha_elector_value( char *name );
int ha_election_state_value( char *name );

curlw_cb ha_watcher_cb;
loop_call_fn ha_watcher_pass;
throw_fn ha_watcher;

loop_call_fn ha_controller_pass;
throw_fn ha_controller;

http_callback ha_get_cluster;
http_callback ha_ctl_cluster;

int ha_init( void );
int ha_start( void );
void ha_shutdown( void );

HAPT *ha_add_partner( char *spec, int dup_fail );
HAPT *ha_find_partner( char *name, int len );

HA_CTL *ha_config_defaults( void );
int ha_config_line( AVP *av );



#endif
