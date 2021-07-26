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
* ha/local.h - defines HA structures and functions                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#ifndef SHARED_HA_LOCAL_H
#define SHARED_HA_LOCAL_H

#define DEFAULT_HA_CHECK_MSEC		1000
#define DEFAULT_HA_UPDATE_MSEC		1000
#define DEFAULT_HA_CHECK_PATH		"/cluster"
#define DEFAULT_HA_CTL_PATH			"/control/cluster"


#include "shared.h"


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


struct ha_string_set
{
	const char			**	arr;
	int						max;

};



extern const char *ha_elector_name_strings[HA_ELECT_MAX];
extern const char *ha_election_state_strings[HA_ELECTION_MAX];
extern const char *ha_clst_line_kind_strings[HA_CLST_LINE_KIND_MAX];
extern const char *ha_clst_line_setting_strings[HA_CLST_LINE_SETT_MAX];
extern struct ha_string_set ha_string_sets[4];


const char *__ha_string_name( int which, int val );
int __ha_string_val( int which, char *str );


#define ha_elector_name( _e )			__ha_string_name( 0, _e )
#define ha_elector_value( _n )			__ha_string_val( 0, _n )

#define ha_election_state_name( _s )	__ha_string_name( 1, _s )
#define ha_election_state_value( _n )	__ha_string_val( 1, _n )

#define ha_clst_line_kind_name( _k )	__ha_string_name( 2, _k )
#define ha_clst_line_kind_value( _n )	__ha_string_val( 2, _n )

#define ha_clst_line_sett_name( _s )	__ha_string_name( 3, _s )
#define ha_clst_line_sett_value( _n )	__ha_string_val( 3, _n )




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

#define lock_ha( )		pthread_mutex_lock(   &(_ha->lock) )
#define unlock_ha( )	pthread_mutex_unlock( &(_ha->lock) )


HAPT *ha_add_partner( char *spec, int dup_fail );
HAPT *ha_find_partner( const char *name, int len );

extern HA_CTL *_ha;

#endif
