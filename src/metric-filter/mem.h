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
#define MEM_ALLOCSZ_FFILE			128

// 1GB
#define DEFAULT_MF_MAX_KB			( 1 * 1024 * 1024 )


struct memt_control
{
	MTYPE			*	hfilt;
	MTYPE			*	ffile;
};


HFILT *mem_new_hfilt( void );
void mem_free_hfilt( HFILT **h );

FFILE *mem_new_ffile( void );
void mem_free_ffile( FFILE **f );
void mem_free_ffile_list( FFILE *list );


MEMT_CTL *memt_config_defaults( void );


#endif
