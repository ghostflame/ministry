/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* target.h - defines target backend structures and functions              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_TEST_TARGET_H
#define MINISTRY_TEST_TARGET_H


#define DEFAULT_MAX_WAITING		128
#define TARGET_MAX_MAX_WAITING	65536


// targets and tsets have these
struct target_io_list
{
	IOLIST				*	head;
	IOLIST				*	end;

	int						bufs;

	pthread_mutex_t			lock;
};


//
// each target is one host/port combination
// it has its own buffer list, enabling it to
// keep track of where it is up to
//
struct target_conf
{
	TARGET				*	next;
	NSOCK				*	sock;
	IOLIST				*	iolist;
	io_fn				*	iofp;
	char				*	host;
	char				*	name;
	char				*	label;

	struct sockaddr_in		sa;

	uint32_t				rc_usec;
	uint32_t				io_usec;

	uint32_t				reconn_ct;
	uint32_t				countdown;
	uint16_t				port;
	int8_t					type;

	// offsets into the current buffer
	int						curr_off;
	int						curr_len;
	int						max;

	// stdout flag
	int						to_stdout;
	int						active;
	int						id;

	pthread_mutex_t			lock;
};



struct target_control
{
	TARGET				*	stats;
	TARGET				*	adder;
	TARGET				*	gauge;
	TARGET				*	compat;
};


throw_fn target_loop;

void target_buf_send( TARGET *t, IOBUF *but );

int target_start( TARGET **tp );
int target_init( void );

TGT_CTL *target_config_defaults( void );
int target_config_line( AVP *av );

#endif
