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
* synth/synth.h - defines synthetics structures                           *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_SYNTH_H
#define MINISTRY_SYNTH_H


struct synth_control
{
	SYNTH			*	list;
	int					scount;
	int					wait_usec;

	int					tcount;
	int					tready;
	int					tproceed;

	pthread_cond_t		threads_ready;
	pthread_cond_t      threads_done;
};


loop_call_fn synth_pass;

throw_fn synth_loop;


void synth_init( void );

SYN_CTL *synth_config_defaults( void );
conf_line_fn synth_config_line;


#endif
