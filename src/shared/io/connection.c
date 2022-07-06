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
		io_disconnect( s, 0 );
		return -1;
	}

	if( err != 0 )
	{
		warn( "I/O socket errored: %s", strerror( err ) );
		io_disconnect( s, 0 );
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
		io_disconnect( s, 1 );
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
		io_disconnect( s, 0 );
		return -1;
	}

	if( connect( s->fd, (struct sockaddr *) &(s->peer), sizeof( struct sockaddr_in ) ) < 0 )
	{
		err( "Unable to connect to %s:%hu -- %s",
			inet_ntoa( s->peer.sin_addr ), ntohs( s->peer.sin_port ),
			Err );
		io_disconnect( s, 0 );
		return -1;
	}

	info( "Connected (%d) to remote host %s:%hu.",
		s->fd, inet_ntoa( s->peer.sin_addr ),
		ntohs( s->peer.sin_port ) );

	s->flags = 0;
	s->connected = 1;

	return s->fd;
}



void io_disconnect( SOCK *s, int sd )
{
	if( s->fd < 0 )
		return;

	if( s->tls )
		io_tls_disconnect( s );

	if( sd && shutdown( s->fd, SHUT_RDWR ) )
		err( "Shutdown error on connection with %s -- %s",
			s->name, Err );

	close( s->fd );
	s->fd = -1;
	s->connected = 0;
}

