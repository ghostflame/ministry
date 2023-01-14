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
* mem.h - defines main memory control structures                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef METRIC_FILTER_MEM_H
#define METRIC_FILTER_MEM_H

#define MEM_ALLOCSZ_HFILT			128
#define MEM_ALLOCSZ_FFILE			128

// 1GB
#define DEFAULT_MF_MAX_KB			( 1 * 1024 * 1024 )


struct memt_control
{
	MTYPE			*	hfilt;
};


HFILT *mem_new_hfilt( void );
void mem_free_hfilt( HFILT **h );

MEMT_CTL *memt_config_defaults( void );


#endif
