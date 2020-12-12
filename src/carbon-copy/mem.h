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


#ifndef CARBON_COPY_MEM_H
#define CARBON_COPY_MEM_H

#define MEM_ALLOCSZ_HBUFS			128
#define MEM_ALLOCSZ_RDATA			128

// 1GB
#define DEFAULT_CC_MAX_KB			( 1 * 1024 * 1024 )


struct memt_control
{
	MTYPE			*	hbufs;
	MTYPE			*	rdata;
};


HBUFS *mem_new_hbufs( void );
void mem_free_hbufs( HBUFS **h );
void mem_free_hbufs_list( HBUFS *list );

RDATA *mem_new_rdata( void );
void mem_free_rdata( RDATA **r );

MEMT_CTL *memt_config_defaults( void );


#endif
