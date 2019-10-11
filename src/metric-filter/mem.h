/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem.h - defines main memory control structures                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef METRIC_FILTER_MEM_H
#define METRIC_FILTER_MEM_H

#define MEM_ALLOCSZ_HFILT			128

// 1GB
#define DEFAULT_MF_MAX_KB			( 1 * 1024 * 1024 )


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
