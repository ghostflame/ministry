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
* filter/init.c - filtering startup functions                             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


void filter_stop( void )
{
	return;
}


int filter_init( void )
{
	int ret;

	pthread_mutex_init( &(ctl->filt->genlock), &(ctl->proc->mem->mtxa) );

	// convert msec to nsec
	ctl->filt->chg_delay *= MILLION;
	ctl->filt->chg_sleep.tv_sec  = ctl->filt->chg_delay / BILLION;
	ctl->filt->chg_sleep.tv_nsec = ctl->filt->chg_delay % BILLION;

	lock_filters( );
	ret = filter_load( );
	unlock_filters( );

	return ret;
}
