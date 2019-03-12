/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* target.h - defines network targets                                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_TARGET_H
#define SHARED_TARGET_H

#define TARGET_PROTO_TCP		IPPROTO_TCP
#define TARGET_PROTO_UDP		IPPROTO_UDP



#ifdef DEBUG
#define tgdebug( fmt, ... )		debug( "[%d:%s] " fmt, t->id, t->name, ##__VA_ARGS__ )
#else
#define tgdebug( fmt, ... )
#endif

#define tginfo( fmt, ... )		info(   "[%d:%s] " fmt, t->id, t->name, ##__VA_ARGS__ )
#define tgnotice( fmt, ... )	notice( "[%d:%s] " fmt, t->id, t->name, ##__VA_ARGS__ )


#ifdef IO_LOCK_SPIN

typedef pthread_spinlock_t io_lock_t;

#define io_lock_init( l )		pthread_spin_init(    &(l), PTHREAD_PROCESS_PRIVATE )
#define io_lock_destroy( l )	pthread_spin_destroy( &(l) )

#define lock_target( t )		pthread_spin_lock(   &(t->lock) )
#define unlock_target( t )		pthread_spin_unlock( &(t->lock) )
#define target_set_id( t )		pthread_spin_lock( &(_proc->io->idlock) ); t->id = ++(_proc->io->tgt_id); pthread_spin_unlock( &(_proc->io->idlock) );

// this is measured against the buffer size; the bitshift should mask off at least buffer size
#define lock_buf( b )			pthread_spin_lock(   &(_proc->io->locks[(((uint64_t) b) >> 6) & _proc->io->lock_mask]) )
#define unlock_buf( b )			pthread_spin_unlock( &(_proc->io->locks[(((uint64_t) b) >> 6) & _proc->io->lock_mask]) )

#else

typedef pthread_mutex_t io_lock_t;

#define io_lock_init( l )		pthread_mutex_init(    &(l), NULL )
#define io_lock_destroy( l )	pthread_mutex_destroy( &(l) )

#define lock_target( t )		pthread_mutex_lock(   &(t->lock) )
#define unlock_target( t )		pthread_mutex_unlock( &(t->lock) )
#define target_set_id( t )		pthread_mutex_lock( &(_proc->io->idlock) ); t->id = ++(_proc->io->tgt_id); pthread_mutex_unlock( &(_proc->io->idlock) );

// this is measured against the buffer size; the bitshift should mask off at least buffer size
#define lock_buf( b )			pthread_mutex_lock(   &(_proc->io->locks[(((uint64_t) b) >> 6) & _proc->io->lock_mask]) )
#define unlock_buf( b )			pthread_mutex_unlock( &(_proc->io->locks[(((uint64_t) b) >> 6) & _proc->io->lock_mask]) )

#endif




struct target
{
	TGT					*	next;
	TGTL				*	list;
	char				*	name;
	char				*	host;
	char				*	typestr;
	char				*	handle;

	// io data
	SOCK				*	sock;
	io_fn				*	iofp;

	// io queue
	IOBP				*	head;
	IOBP				*	tail;
	int32_t					curr;
	int32_t					max;

	// current buffer
	int32_t					curr_off;
	int32_t					curr_len;

	// reconnecting - tcp only
	int32_t					rc_count;
	int32_t					rc_limit;

	// misc
	io_lock_t				lock;
	int						to_stdout;
	int						enabled;
	int						proto;
	int32_t					id;
	uint16_t				port;

	// for callers to fill in
	void				*	ptr;
	int						type;
};

struct target_list
{
	TGTL				*	next;
	char				*	name;

	TGT					*	targets;
	int						count;
};


struct target_control
{
	TGTL				*	lists;
	int						count;
	int						tcount;

	target_cfg_fn		*	type_fn;
	target_cfg_fn		*	data_fn;
};



// run targets (list is optional)
int target_run_one( TGT *t, int enabled_check, int idx );
int target_run_list( TGTL *list, int enabled_check );
int target_run( int enabled_check );

TGTL *target_list_all( void );
TGTL *target_list_find( char *name );
TGTL *target_list_create( char *name );

TGT *target_create( char *list, char *name, char *proto, char *host, uint16_t port, char *type, int enabled );
TGT *target_create_str( char *src, int len, char sep );

void target_set_type_fn( target_cfg_fn *fp );
void target_set_data_fn( target_cfg_fn *fp );

throw_fn target_loop;

// if prefix is null we use name
void target_set_handle( TGT *t, char *prefix );

TGT_CTL *target_config_defaults( void );
int target_config_line( AVP *av );

#endif
