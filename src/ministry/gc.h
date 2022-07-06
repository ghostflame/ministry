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
* gc.h - declares some gc routines                                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_GC_H
#define MINISTRY_GC_H

#define DEFAULT_GC_THRESH           8640        // 1 day @ 10s
#define DEFAULT_GC_GG_THRESH        25920       // 3 days @ 10s


struct gc_control
{
	int64_t					enabled;
	int64_t					thresh;
	int64_t					gg_thresh;
};


loop_call_fn gc_pass;
throw_fn gc_loop;
conf_line_fn gc_config_line;

GC_CTL *gc_config_defaults( void );

#endif
