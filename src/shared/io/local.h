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

