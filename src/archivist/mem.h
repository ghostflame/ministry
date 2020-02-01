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

#define MEM_ALLOCSZ_TREEL			128		// 64b, so 8k
#define MEM_ALLOCSZ_TLEAF			128		// ?
#define MEM_ALLOCSZ_QRYPT			256		// 16b, so 4k

// 4GB
#define DEFAULT_RK_MAX_KB			( 4 * 1024 * 1024 )


struct memt_control
{
	MTYPE			*	treel;
	MTYPE			*	tleaf;
	MTYPE			*	qrypt;
};


TEL *mem_new_treel( char *str, int len );
void mem_free_treel( TEL **t );
void mem_free_treel_branch( TEL *t );

LEAF *mem_new_tleaf( void );
void mem_free_tleaf( LEAF **l );

QP *mem_new_qrypt( void );
void mem_free_qrypt( QP **q );
void mem_free_qrypt_LIST( QP *list );

MEMT_CTL *memt_config_defaults( void );


#endif
