/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem.h - defines main memory control structures                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef CARBON_COPY_MEM_H
#define CARBON_COPY_MEM_H

#define MEM_ALLOCSZ_HOSTS			128
#define MEM_ALLOCSZ_HBUFS			128



struct memt_control
{
	MTYPE			*	hosts;
	MTYPE			*	hbufs;
};


HOST *mem_new_host( struct sockaddr_in *peer, uint32_t bufsz );
void mem_free_host( HOST **h );

HBUFS *mem_new_hbufs( void );
void mem_free_hbufs( HBUFS **h );
void mem_free_hbufs_list( HBUFS *list );

MEMT_CTL *memt_config_defaults( void );


#endif
