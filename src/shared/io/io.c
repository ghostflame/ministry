/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* io/io.c - handles network i/o and buffered writes                       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



void io_sock_set_peer( SOCK *s, struct sockaddr_in *peer )
{
	if( !s || !peer )
		return;

	s->peer = *peer;

	if( !s->name )
		s->name = perm_str( 32 );

	snprintf( s->name, 32, "%s:%hu", inet_ntoa( peer->sin_addr ),
		ntohs( peer->sin_port ) );
}



SOCK *io_make_sock( int32_t insz, int32_t outsz, struct sockaddr_in *peer )
{
	SOCK *s;

	if( !( s = (SOCK *) allocz( sizeof( SOCK ) ) ) )
		fatal( "Could not allocate new nsock." );

	// copy the peer contents
	io_sock_set_peer( s, peer );

	if( insz )
		s->in = mem_new_iobuf( insz );

	if( outsz )
		s->out = mem_new_iobuf( outsz );

	// no socket yet
	s->fd = -1;

	return s;
}



// processing control


int io_init( void )
{
	IO_CTL *i = _io;
	uint64_t k;

	// allocate and init our locks
	i->lock_size = 1 << i->lock_bits;
	i->lock_mask = i->lock_size - 1;

	i->locks = (io_lock_t *) allocz( i->lock_size * sizeof( io_lock_t ) );

	for( k = 0; k < i->lock_size; k++ )
	{
		io_lock_init( i->locks[k] );
	}

	io_lock_init( i->idlock );

	return 0;
}


void io_stop( void )
{
	IO_CTL *i = _io;
	uint64_t k;

	for( k = 0; k < i->lock_size; k++ )
	{
		io_lock_destroy( i->locks[k] );
	}
}


