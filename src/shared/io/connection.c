/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* io/connection.c - network i/o connection functions                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"


int io_connected( SOCK *s )
{
	socklen_t len;
	int err;

	if( s->fd < 0 )
	{
		debug( "Not connected, aborting check." );
		return -1;
	}

	err = 0;
	len = sizeof( int );

	if( getsockopt( s->fd, SOL_SOCKET, SO_ERROR, &err, &len ) < 0 )
	{
		warn( "Could not assess target socket state: %s", Err );
		close( s->fd );
		s->fd = -1;
		s->connected = 0;

		return -1;
	}

	if( err != 0 )
	{
		warn( "I/O socket errored: %s", strerror( err ) );
		close( s->fd );
		s->fd = -1;
		s->connected = 0;

		return -1;
	}

	return 0;
}



int io_connect( SOCK *s )
{
	int opt = 1;

	if( s->fd != -1 )
	{
		warn( "Net connect called on connected socket - disconnecting." );
		shutdown( s->fd, SHUT_RDWR );
		close( s->fd );
		s->fd = -1;
		s->connected = 0;
	}

	if( ( s->fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
	{
		err( "Unable to make tcp socket for %s -- %s", s->name, Err );
		return -1;
	}

	if( setsockopt( s->fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof( int ) ) )
	{
		err( "Unable to set keepalive on socket for %s -- %s",
			s->name, Err );
		close( s->fd );
		s->fd = -1;
		return -1;
	}

	if( connect( s->fd, (struct sockaddr *) &(s->peer), sizeof( struct sockaddr_in ) ) < 0 )
	{
		err( "Unable to connect to %s:%hu -- %s",
			inet_ntoa( s->peer.sin_addr ), ntohs( s->peer.sin_port ),
			Err );
		close( s->fd );
		s->fd = -1;
		return -1;
	}

	info( "Connected (%d) to remote host %s:%hu.",
		s->fd, inet_ntoa( s->peer.sin_addr ),
		ntohs( s->peer.sin_port ) );

	s->flags = 0;
	s->connected = 1;

	return s->fd;
}



void io_disconnect( SOCK *s )
{
	if( s->fd < 0 )
		return;

	if( shutdown( s->fd, SHUT_RDWR ) )
		err( "Shutdown error on connection with %s -- %s",
			s->name, Err );

	close( s->fd );
	s->fd = -1;
	s->connected = 0;
}

