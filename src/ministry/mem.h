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


#ifndef MINISTRY_MEM_H
#define MINISTRY_MEM_H

#define MEM_ALLOCSZ_POINTS			2048
#define MEM_ALLOCSZ_DHASH			512
#define MEM_ALLOCSZ_PREDS			128
#define MEM_ALLOCSZ_HISTY			128
#define MEM_ALLOCSZ_METRY			128

struct memt_control
{
	MTYPE			*	points;
	MTYPE			*	dhash;
	MTYPE			*	preds;
	MTYPE			*	histy;
	MTYPE			*	metry;
};


PTLIST *mem_new_points( void );
void mem_free_points( PTLIST **p );
void mem_free_points_list( PTLIST *list );

DHASH *mem_new_dhash( const char *str, int len );
void mem_free_dhash( DHASH **d );
void mem_free_dhash_list( DHASH *list );

PRED *mem_new_pred( void );
void mem_free_pred( PRED **p );
void mem_free_pred_list( PRED *list );

HIST *mem_new_history( uint16_t size );
void mem_free_history( HIST **h );
void mem_free_history_list( HIST *list );

METRY *mem_new_metry( const char *str, int len );
void mem_free_metry( METRY **m );
void mem_free_metry_list( METRY *list );

MEMT_CTL *memt_config_defaults( void );


#endif
