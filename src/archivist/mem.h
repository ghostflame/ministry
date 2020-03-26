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

#define MEM_ALLOCSZ_QRYPT			256		// 16b, so 4k
#define MEM_ALLOCSZ_RKQRY			256

// 4GB
#define DEFAULT_RK_MAX_KB			( 6 * 1024 * 1024 )


struct memt_control
{
	MTYPE			*	qrypt;
	MTYPE			*	rkqry;
};


QP *mem_new_qrypt( void );
void mem_free_qrypt( QP **q );
void mem_free_qrypt_list( QP *list );

RKQR *mem_new_rkqry( void );
void mem_free_rkqry( RKQR **r );
void mem_free_rkqry_list( RKQR *list );

MEMT_CTL *memt_config_defaults( void );


#endif
