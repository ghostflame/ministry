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
* monitors.c - read config and start monitors                             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "min-monitor.h"


int monitors_init( void )
{
	ctl->mons->intv = MILLION * ctl->mons->intv_msec;

	return 0;
}


MONCFG *monitors_config_defaults( void )
{
	MONCFG *mc = (MONCFG *) mem_perm( sizeof( MONCFG ) );

	mc->mondir    = str_copy( DEFAULT_MONITORS_DIR, 0 );
	mc->intv_msec = DEFAULT_INTERVAL_MSEC;
	mc->max_ctr   = DEFAULT_MAX_MONITORS;

	return mc;
}


int monitors_config_line( AVP *av )
{
	return -1;
}

