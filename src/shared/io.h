/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* io.h - defines network buffers and io routines                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_IO_H
#define SHARED_IO_H

#define IO_BUF_SZ				0x40000		// 256k
#define IO_BUF_HWMK				0x3c000		// 240k
#define IO_MAX_WAITING			1024		// makes for 1024 * 256k = 256M
#define IO_LIM_WAITING			65536		// makes for 16G

#define IO_CLOSE				0x01
#define IO_CLOSE_EMPTY			0x02

#define IO_BUF_FLG_INIT			0x01

#define IO_BLOCK_BITS			5	// 32 locks

#define IO_SEND_USEC			4000	// usec
#define IO_RECONN_DELAY			2000	// msec


// this is measured against the buffer size; the bitshift should mask off at least buffer size
#define lock_buf( b )			pthread_spin_lock(   &(_io->locks[(((uint64_t) b) >> 6) & _io->lock_mask]) )
#define unlock_buf( b )			pthread_spin_unlock( &(_io->locks[(((uint64_t) b) >> 6) & _io->lock_mask]) )



// size: (8x8) 64
struct io_buffer
{
	IOBUF			*	next;
	char			*	ptr;		// holds memory even if not requested
	char			*	buf;
	int16_t				refs;		// how many outstanding to send?
	int16_t				flags;
	int32_t				hwmk;
	int32_t				len;
	int32_t				sz;
	int64_t				mtime;
	int64_t				expires;
	int64_t				lifetime;
};


struct io_buf_ptr
{
	IOBP			*	next;
	IOBP			*	prev;
	IOBUF			*	buf;
	int64_t				_unused;
};


struct io_socket
{
	IOBUF				*	out;
	IOBUF				*	in;

	int						fd;
	int						flags;
	int						bufs;
	int						proto;

	struct sockaddr_in		peer;
	char				*	name;
};


struct io_control
{
	pthread_spinlock_t	*	locks;

	uint64_t				lock_bits;
	uint64_t				lock_size;
	uint64_t				lock_mask;

	int32_t					send_usec;
	int32_t					rc_msec;

	int32_t					tgt_id;
	pthread_spinlock_t		idlock;

	int						stdout_count;
};



// r/w
int io_read_data( SOCK *s );
int io_write_data( SOCK *s, int off );

// connection
int  io_connected( SOCK *s );
int  io_connect( SOCK *s );
void io_disconnect( SOCK *s );

// buffers
void io_buf_post_one( TGT *t, IOBUF *buf );
void io_buf_post( TGTL *l, IOBUF *buf );

// io fns
io_fn io_send_net_tcp;
io_fn io_send_net_udp;
io_fn io_send_stdout;
io_fn io_send_file;

throw_fn io_target_loop;

// sockets
SOCK *io_make_sock( int32_t insz, int32_t outsz, struct sockaddr_in *peer );

// startup/shutdown
int io_init( void );
void io_stop( void );

IO_CTL *io_config_defaults( void );
int io_config_line( AVP *av );


#endif
