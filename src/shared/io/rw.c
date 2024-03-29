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
* io/rw.c - network io send and recv functions                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


__attribute__((hot)) int io_read_data( SOCK *s )
{
	int i;

	if( !( i = recv( s->fd, s->in->bf->buf + s->in->bf->len, s->in->bf->sz - ( s->in->bf->len + 2 ), MSG_DONTWAIT ) ) )
	{
		if( s->flags & IO_CLOSE_EMPTY )
		{
			// that would be the fin, then
			debug( "Received a FIN, perhaps, from %s", s->name );
			flagf_add( s, IO_CLOSE );
		}
		return 0;
	}
	else if( i < 0 )
	{
		if( errno != EAGAIN
		 && errno != EWOULDBLOCK )
		{
			err( "Recv error for host %s -- %s",
				s->name, Err );
			flagf_add( s, IO_CLOSE );
			return i;
		}
		return 0;
	}

	// got some data then
	s->in->bf->len += i;

	//debug( "Received %d bytes on socket %d/%s", i, s->fd, s->name );

	return i;
}




// we have to pass an offset in because
// we cannot modify the buffer, else later
// threads writing the same buffer will
// perceive it to be empty
int io_write_data( SOCK *s, int off )
{
	int rv, len, wr, tries, sent;
	struct pollfd p;
	char *ptr;
	IOBUF *b;

	p.fd     = s->fd;
	p.events = POLLOUT;
	tries    = 3;
	sent     = 0;
	b        = s->out;
	ptr      = b->bf->buf + off;
	len      = b->bf->len - off;

	while( len > 0 )
	{
		if( ( rv = poll( &p, 1, 20 ) ) < 0 )
		{
			if( errno == EINTR )
				continue;

			warn( "Poll error writing to host %s -- %s",
				s->name, Err );
			flagf_add( s, IO_CLOSE );
			break;
		}

		if( !rv )
		{
			// wait another 20msec and try again
			if( tries-- > 0 )
				continue;

			// we cannot write just yet
			break;
		}

		if( ( wr = send( s->fd, ptr, len, MSG_NOSIGNAL ) ) < 0 )
		{
			warn( "Error writing to host %s -- %s",
				s->name, Err );
			flagf_add( s, IO_CLOSE );
			break;
		}

		len  -= wr;
		ptr  += wr;
		sent += wr;
	}

	// what we wrote - adds to offset
	return sent;
}



