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

typedef pthread_spinlock_t target_lock_t;

#define lock_target( t )		pthread_spin_lock(   &(t->lock) )
#define unlock_target( t )		pthread_spin_unlock( &(t->lock) )

#define target_set_id( t )		pthread_spin_lock( &(_io->idlock) ); t->id = ++(_io->tgt_id); pthread_spin_unlock( &(_io->idlock) ); 


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
	target_lock_t			lock;
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

TGTL *target_list_find( char *name );
TGTL *target_list_create( char *name );

TGT *target_create( char *list, char *name, char *proto, char *host, uint16_t port, char *type, int enabled );
TGT *target_create_str( char *src, int len, char sep );

void target_set_type_fn( target_cfg_fn *fp );
void target_set_data_fn( target_cfg_fn *fp );

// if prefix is null we use name
void target_set_handle( TGT *t, char *prefix );

TGT_CTL *target_config_defaults( void );
int target_config_line( AVP *av );

#endif
