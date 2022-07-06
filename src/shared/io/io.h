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
* io.h - defines network buffers and io routines                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_IO_H
#define SHARED_IO_H

#define IO_CLOSE				0x0001
#define IO_CLOSE_EMPTY			0x0002
#define IO_TLS					0x1000
#define IO_TLS_VERIFY			0x2000
#define IO_TLS_MASK				0xf000

#define IO_BUF_SZ				0x40000		// 256k
#define IO_BUF_HWMK				0x3c000		// 240k

#define IO_BUF_SMALL			0x4000		// 16k

#define IO_MAX_WAITING			1024		// makes for 1024 * 256k = 256M
#define IO_LIM_WAITING			65536		// makes for 16G




struct io_tls
{
	gnutls_datum_t						dt;
	gnutls_session_t					sess;
	gnutls_certificate_credentials_t	cred;
	char							*	peer;
	uint32_t							flags;
	int16_t								plen;
	int8_t								init;
	int8_t								conn;
};


// size: (6x8) 48
struct io_buffer
{
	IOBUF			*	next;
	BUF				*	bf;
	int16_t				refs;		// how many outstanding to send?
	int16_t				flags;
	uint32_t			hwmk;
	int64_t				mtime;
	int64_t				expires;
	int64_t				lifetime;
};



struct io_socket
{
	IOBUF				*	out;
	IOBUF				*	in;

	IOTLS				*	tls;

	int						fd;
	int						flags;
	int						bufs;
	int						proto;
	int64_t					connected;

	struct sockaddr_in		peer;
	char				*	name;
};


struct io_control
{
	io_lock_t			*	locks;

	uint64_t				lock_bits;
	uint64_t				lock_size;
	uint64_t				lock_mask;

	int32_t					send_usec;
	int32_t					rc_msec;

	int32_t					tgt_id;
	io_lock_t				idlock;

	int						stdout_count;
	int						tls_init;
};



// r/w
int io_read_data( SOCK *s );
void io_disconnect( SOCK *s, int sd );

// buffers
void io_buf_post_one( TGT *t, IOBUF *buf );
void io_buf_post( TGTL *l, IOBUF *buf );

// io fns
io_fn io_send_net_tcp;
io_fn io_send_net_udp;
io_fn io_send_net_tls;
io_fn io_send_stdout;
io_fn io_send_file;

// sockets
SOCK *io_make_sock( int32_t insz, int32_t outsz, struct sockaddr_in *peer, uint32_t flags, char *peername );
void io_sock_set_peer( SOCK *s, struct sockaddr_in *peer );

// startup/shutdown
int io_init( void );
void io_stop( void );

IO_CTL *io_config_defaults( void );
conf_line_fn io_config_line;


#endif
