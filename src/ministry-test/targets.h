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
* target.h - defines target backend structures and functions              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_TEST_TARGET_H
#define MINISTRY_TEST_TARGET_H

#define DEFAULT_STATS_PORT				9125
#define DEFAULT_ADDER_PORT				9225
#define DEFAULT_GAUGE_PORT				9325
#define DEFAULT_HISTO_PORT				9425
#define DEFAULT_COMPAT_PORT				8125

#define DEFAULT_HOST					"127.0.0.1"


struct targets_defaults
{
	int						type;
	uint16_t				port;
	char				*	name;
};


struct targets_control
{
	TGT					*	targets[METRIC_TYPE_MAX];
};



target_cfg_fn targets_set_type;

int targets_start_one( TGT **tp );
void targets_resolve( void );

TGTS_CTL *targets_config_defaults( void );

#endif

