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
* synth/local.h - defines synthetics structures                           *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_SYNTH_LOCAL_H
#define MINISTRY_SYNTH_LOCAL_H

#define SYNTH_PART_MAX			32


#include "ministry.h"


// just used for parsing config lines
struct synth_fn_def
{
	char			*	names[3];
	synth_fn		*	fn;
	int					min_parts;
	int					max_parts;
	int					max_absent;
};


struct synth_data
{
	SYNTH			*	next;

	DHASH			*	dhash[SYNTH_PART_MAX];
	char			*	paths[SYNTH_PART_MAX];
	int					plens[SYNTH_PART_MAX];

	DHASH			*	target;
	char			*	target_path;

	double				factor;			// defaults to 1

	int					enable;
	int					parts;
	int					missing;

	int					absent;
	int					max_absent;

	SYNDEF			*	def;
};



synth_fn synth_sum;
synth_fn synth_diff;
synth_fn synth_div;
synth_fn synth_prod;
synth_fn synth_max;
synth_fn synth_min;
synth_fn synth_cap;
synth_fn synth_spread;
synth_fn synth_mean;
synth_fn synth_meanIf;
synth_fn synth_count;
synth_fn synth_active;


extern SYN_CTL *_syn;

#endif
