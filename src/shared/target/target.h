/**************************************************************************
* Copyright 2015 John Denholm                                             *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
*                                                                         *
*                                                                         *
* target.h - defines network targets                                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_TARGET_H
#define SHARED_TARGET_H

#define tgdebug( fmt, ... )		debug(  "[%d:%s] " fmt, t->id, t->name, ##__VA_ARGS__ )
#define tginfo( fmt, ... )		info(   "[%d:%s] " fmt, t->id, t->name, ##__VA_ARGS__ )
#define tgnotice( fmt, ... )	notice( "[%d:%s] " fmt, t->id, t->name, ##__VA_ARGS__ )
#define tgwarn( fmt, ... )		warn(   "[%d:%s] " fmt, t->id, t->name, ##__VA_ARGS__ )
#define tgerr( fmt, ... )		err(    "[%d:%s] " fmt, t->id, t->name, ##__VA_ARGS__ )



#ifdef IO_LOCK_SPIN

typedef pthread_spinlock_t io_lock_t;

#define io_lock_init( l )		pthread_spin_init(    &(l), PTHREAD_PROCESS_PRIVATE )
#define io_lock_destroy( l )	pthread_spin_destroy( &(l) )

#define target_set_id( t )		pthread_spin_lock( &(_proc->io->idlock) ); t->id = ++(_proc->io->tgt_id); pthread_spin_unlock( &(_proc->io->idlock) );

// this is measured against the buffer size; the bitshift should mask off at least buffer size
#define lock_buf( b )			pthread_spin_lock(   &(_proc->io->locks[(((uint64_t) b) >> 6) & _proc->io->lock_mask]) )
#define unlock_buf( b )			pthread_spin_unlock( &(_proc->io->locks[(((uint64_t) b) >> 6) & _proc->io->lock_mask]) )

#else

typedef pthread_mutex_t io_lock_t;

#define io_lock_init( l )		pthread_mutex_init(    &(l), NULL )
#define io_lock_destroy( l )	pthread_mutex_destroy( &(l) )

#define target_set_id( t )		pthread_mutex_lock( &(_proc->io->idlock) ); t->id = ++(_proc->io->tgt_id); pthread_mutex_unlock( &(_proc->io->idlock) );

// this is measured against the buffer size; the bitshift should mask off at least buffer size
#define lock_buf( b )			pthread_mutex_lock(   &(_proc->io->locks[(((uint64_t) b) >> 6) & _proc->io->lock_mask]) )
#define unlock_buf( b )			pthread_mutex_unlock( &(_proc->io->locks[(((uint64_t) b) >> 6) & _proc->io->lock_mask]) )

#endif



#define TGT_FLAG_ENABLED		0x00000001
#define TGT_FLAG_STDOUT			0x00000002
#define TGT_FLAG_TLS			IO_TLS
#define TGT_FLAG_TLS_VERIFY		IO_TLS_VERIFY


struct target
{
	TGT					*	next;
	TGTL				*	list;
	char				*	name;
	char				*	host;
	char				*	path;
	char				*	typestr;
	char				*	handle;

	// io data
	SOCK				*	sock;
	FILE				*	fh;
	io_fn				*	iofp;

	// metrics data
	PMET				*	pm_bytes;
	PMET				*	pm_conn;
	PMET_LBL			*	pm_lbls;

	// io queue
	MEMHL				*	queue;
	int64_t					max;

	// current buffer
	int32_t					curr_off;
	int32_t					curr_len;

	// reconnecting - tcp only
	int32_t					rc_count;
	int32_t					rc_limit;

	// misc
	int64_t					bytes;
	uint32_t				flags;
	uint16_t				port;
	int16_t					nlen;
	int32_t					id;
	int8_t					proto;

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
	int						enabled;
};


struct target_metrics
{
	PMETS				*	source;
	PMETM				*	bytes;
	PMETM				*	conn;
};


struct target_control
{
	TGTL				*	lists;
	int						count;
	int						tcount;

	TGTMT				*	metrics;

	target_cfg_fn		*	type_fn;
	target_cfg_fn		*	data_fn;
};


// write to all targets
void target_write_all( IOBUF *buf );

// run targets (list is optional)
int target_run_one( TGT *t, int idx );
int target_run_list( TGTL *list );
int target_run( void );

TGTL *target_list_all( void );
TGTL *target_list_find( char *name );
TGTL *target_list_create( char *name );
TGT *target_list_search( TGTL *l, char *name, int len );
void target_list_check_enabled( TGTL *l );

TGT *target_create( char *list, char *name, char *proto, char *host, uint16_t port, char *type, int enabled );
TGT *target_create_str( char *src, int len, char sep );

void target_set_type_fn( target_cfg_fn *fp );
void target_set_data_fn( target_cfg_fn *fp );

http_callback target_http_toggle;
http_callback target_http_list;

throw_fn target_loop;

// if prefix is null we use name
void target_set_handle( TGT *t, char *prefix );

TGT_CTL *target_config_defaults( void );
conf_line_fn target_config_line;

#endif
