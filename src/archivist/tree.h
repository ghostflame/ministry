/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* tree.h - main data tree structure definitions                           *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef ARCHIVIST_TREE_H
#define ARCHIVIST_TREE_H


struct tree_leaf
{
	char			*	filepath;	// ?
	int					fd;

	int64_t				last_updated;

	TEL				*	tel;

	PTS				*	points;
};


typedef pthread_mutex_t tree_lock_t;

#define tree_lock( _t )			pthread_mutex_lock(   &(_t->lock) )
#define tree_unlock( _t )		pthread_mutex_unlock( &(_t->lock) )

#define tree_lock_init( _t )	pthread_mutex_init(    &(_t->lock), NULL )
#define tree_lock_dest( _t )	pthread_mutex_destroy( &(_t->lock) )

#define tree_tid_lock( )		pthread_mutex_lock(   &(_tree->tid_lock) )
#define tree_tid_unlock( )		pthread_mutex_unlock( &(_tree->tid_lock) )


// 80-84 bytes
struct tree_element
{
	TEL				*	next;
	TEL				*	child;
	TEL				*	parent;

	SSTE			*	he;

	char			*	name;
	char			*	path;
	LEAF			*	leaf;

	tree_lock_t			lock;

	uint16_t			len;
	uint16_t			plen;
	uint32_t			id;
};




struct tree_control
{
	TEL				*	root;
	SSTR			*	hash;		// lookup table

	int64_t				hashsz;		// get from config

	uint32_t			tid;
	pthread_mutex_t		tid_lock;
};


LEAF *tree_process_line( char *str, int len );

int tree_init( void );
TREE_CTL *tree_config_defaults( void );
conf_line_fn tree_config_line;


#endif
