/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* io/local.h - unexported io structures and function declarations         *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_IO_LOCAL_H
#define SHARED_IO_LOCAL_H



#define IO_BUF_FLG_INIT			0x01

#define IO_BLOCK_BITS			5	// 32 locks

#define IO_SEND_USEC			11000	// usec
#define IO_RECONN_DELAY			2000	// msec




#include "shared.h"




// buffers
void io_buf_next( TGT *t );
void io_buf_decr( IOBUF *buf );


int io_write_data( SOCK *s, int off );
int io_connected( SOCK *s );
int io_connect( SOCK *s );

// tls
IOTLS *io_tls_make_session( uint32_t flags, char *peername );
void io_tls_end_session( SOCK *s );
int io_tls_write_data( SOCK *s, int off );
int io_tls_connect( SOCK *s );
int io_tls_disconnect( SOCK *s );

// the local variable
extern IO_CTL *_io;


#endif

