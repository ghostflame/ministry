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
#define MEM_ALLOCSZ_PTLST			128		// 2k x 128, so 256k
#define MEM_ALLOCSZ_RKQRY			256

// 4GB
#define DEFAULT_RK_MAX_KB			( 6 * 1024 * 1024 )


struct memt_control
{
	MTYPE			*	treel;
	MTYPE			*	tleaf;
	MTYPE			*	qrypt;
	MTYPE			*	ptlst;
	MTYPE			*	rkqry;
};


TEL *mem_new_treel( char *str, int len );
void mem_free_treel( TEL **t );
void mem_free_treel_branch( TEL *t );

LEAF *mem_new_tleaf( void );
void mem_free_tleaf( LEAF **l );

QP *mem_new_qrypt( void );
void mem_free_qrypt( QP **q );
void mem_free_qrypt_list( QP *list );

PTS *mem_new_ptlst( void );
void mem_free_ptlst( PTS **p );
void mem_free_ptlst_list( PTS *list );

RKQR *mem_new_rkqry( void );
void mem_free_rkqry( RKQR **r );
void mem_free_rkqry_list( RKQR *list );


MEMT_CTL *memt_config_defaults( void );


#endif
