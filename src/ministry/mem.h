/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
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


PTLIST *mem_new_point( void );
void mem_free_point( PTLIST **p );
void mem_free_point_list( PTLIST *list );

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
