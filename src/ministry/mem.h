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

#define MEM_ALLOCSZ_HOSTS			128
#define MEM_ALLOCSZ_POINTS			512
#define MEM_ALLOCSZ_DHASH			512
#define MEM_ALLOCSZ_TOKEN			128

#define DEFAULT_GC_THRESH			8640		// 1 day @ 10s
#define DEFAULT_GC_GG_THRESH		25920		// 3 days @ 10s


struct memt_control
{
	MTYPE			*	hosts;
	MTYPE			*	points;
	MTYPE			*	dhash;
	MTYPE			*	token;

	int64_t				gc_enabled;
	int64_t				gc_thresh;
	int64_t				gc_gg_thresh;
};


HOST *mem_new_host( struct sockaddr_in *peer, uint32_t bufsz );
void mem_free_host( HOST **h );

PTLIST *mem_new_point( void );
void mem_free_point( PTLIST **p );
void mem_free_point_list( PTLIST *list );

DHASH *mem_new_dhash( char *str, int len );
void mem_free_dhash( DHASH **d );
void mem_free_dhash_list( DHASH *list );

TOKEN *mem_new_token( void );
void mem_free_token( TOKEN **t );
void mem_free_token_list( TOKEN *list );

int memt_config_line( AVP *av );
MEMT_CTL *memt_config_defaults( void );


#endif
