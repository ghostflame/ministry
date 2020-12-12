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
* ha/ha.c - high-availability server functions                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#include "local.h"


const char *ha_elector_name_strings[HA_ELECT_MAX] =
{
	"random", "first", "manual"
};

const char *ha_election_state_strings[HA_ELECTION_MAX] =
{
	"config", "choosing", "chosen", "mismatch"
};

const char *ha_clst_line_kind_strings[HA_CLST_LINE_KIND_MAX] =
{
	"STATUS", "SETTING", "PARTNER"
};

const char *ha_clst_line_setting_strings[HA_CLST_LINE_SETT_MAX] =
{
	"ELECTOR", "PERIOD", "UPDATE"
};


struct ha_string_set ha_string_sets[4] =
{
	{	ha_elector_name_strings,		HA_ELECT_MAX			},
	{	ha_election_state_strings,		HA_ELECTION_MAX			},
	{	ha_clst_line_kind_strings,		HA_CLST_LINE_KIND_MAX	},
	{	ha_clst_line_setting_strings,	HA_CLST_LINE_SETT_MAX	}
};


const char *__ha_string_name( int which, int val )
{
	struct ha_string_set *hss = ha_string_sets + which;

	if( val < 0 || val >= hss->max )
		return NULL;

	return hss->arr[val];
}

int __ha_string_val( int which, char *str )
{
	struct ha_string_set *hss = ha_string_sets + which;
	int i;

	for( i = 0; i < hss->max; ++i )
		if( !strcasecmp( str, hss->arr[i] ) )
			return i;

	return -1;
}



