/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* io.h - defines network buffers and io routines                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_TEST_IO_H
#define MINISTRY_TEST_IO_H


#define IO_BUF_SZ				0x40000		// 256k
#define IO_BUF_HWMK				0x3c000		// 240k
#define IO_MAX_WAITING			1024		// makes for 1024 * 256k = 256M


#define IO_CLOSE				0x0001



struct io_buffer
{
	IOBUF			*	next;
	IOBUF			*	prev;
	char			*	ptr;		// holds memory even if not requested
	char			*	buf;
	int64_t				expires;
	int64_t				lifetime;
	int					len;
	int					sz;
	int					inited;
};

struct io_buffer_list
{
	IOLIST			*	next;

	IOBUF			*	head;
	IOBUF			*	tail;

	int					bufs;
	int					inited;

	pthread_mutex_t		lock;
};



// r/w
int io_write_data( NSOCK *s, int off );

int io_connected( NSOCK *s );
int io_connect( TARGET *t );

void io_decr_buf( IOBUF *buf );

void io_post_buffer( IOLIST *t, IOBUF *buf );
IOBUF *io_fetch_buffer( IOLIST *t );

io_fn io_send_net;
io_fn io_send_stdout;

throw_fn io_loop;


#endif
