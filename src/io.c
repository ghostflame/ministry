#include "ministry.h"


// does not have it's own config, runs off mem.c config


int io_read_data( NSOCK *s )
{
	int i;

	if( s->keep->len )
	{
	  	// can we shift the kept string
		if( ( s->keep->buf - s->in->buf ) >= s->keep->len )
			memcpy( s->in->buf, s->keep->buf, s->keep->len );
		else
		{
			// no, we can't
			char *p, *q;

			i = s->keep->len;
			p = s->in->buf;
			q = s->keep->buf;

			while( i-- > 0 )
				*p++ = *q++;
		}

		s->keep->buf = NULL;
		s->in->len   = s->keep->len;
		s->keep->len = 0;
	}
	else
		s->in->len = 0;

	if( !( i = recv( s->sock, s->in->buf + s->in->len, s->in->sz - ( s->in->len + 2 ), MSG_DONTWAIT ) ) )
	{
		// that would be the fin, then
		debug( "Received a FIN, perhaps, from %s", s->name );
		s->flags |= HOST_CLOSE;
		return 0;
	}
	else if( i < 0 )
	{
		if( errno != EAGAIN
		 && errno != EWOULDBLOCK )
		{
			err( "Recv error for host %s -- %s",
				s->name, Err );
			s->flags |= HOST_CLOSE;
			return i;
		}
		return 0;
	}

	// got some data then
	s->in->len += i;

	//debug( "Received %d bytes on socket %d/%s", i, s->sock, s->name );

	return i;
}


int io_read_lines( HOST *h )
{
	int i, keeplast = 0, l;
	NSOCK *n = h->net;
	char *w;

	// try to read some data
	if( ( i = io_read_data( n ) ) <= 0 )
		return i;

	// do we have anything at all?
	if( !n->in->len )
	{
		debug( "No incoming data from %s", n->name );
		return 0;
	}

	// mark the socket as active
	h->last = ctl->curr_time.tv_sec;

	if( n->in->buf[n->in->len - 1] == LINE_SEPARATOR )
		// remove any trailing separator
		n->in->buf[--(n->in->len)] = '\0';
	else
	 	// make a note to keep the last line back
		keeplast = 1;

	if( strwords( h->all, n->in->buf, n->in->len, LINE_SEPARATOR ) < 0 )
	{
		debug( "Invalid buffer from data host %s.", n->name );
		return -1;
	}

	// clean \r's
	for( i = 0; i < h->all->wc; i++ )
	{
		w = h->all->wd[i];
		l = h->all->len[i];

		// might be at the start
		if( *w == '\r' )
		{
			++(h->all->wd[i]);
			--(h->all->len[i]);
			--l;
		}

		// remove trailing carriage returns
		if( *(w + l - 1) == '\r' )
			h->all->wd[--(h->all->len[i])] = '\0';
	}



	// claw back the last line - it was incomplete
	if( h->all->wc && keeplast )
	{
		// if we have several lines we can't move the last line
		if( --(h->all->wc) )
		{
			// move it next time
			n->keep->buf = h->all->wd[h->all->wc];
			n->keep->len = h->all->len[h->all->wc];
		}
		else
		{
			// it's the only line
			n->in->len   = h->all->len[0];
			n->keep->buf = NULL;
			n->keep->len = 0;
		}
	}

	return h->all->wc;
}



int io_write_data( NSOCK *s )
{
	int rv, len, wr, tries, sent;
	struct pollfd p;
	char *ptr;
	IOBUF *b;

	p.fd     = s->sock;
	p.events = POLLOUT;
	tries    = 3;
	sent     = 0;
	b        = s->out;
	ptr      = b->buf;
	len      = b->len;


	while( len > 0 )
	{
		if( ( rv = poll( &p, 1, 20 ) ) < 0 )
		{
			warn( "Poll error writing to host %s -- %s",
				s->name, Err );
			s->flags |= HOST_CLOSE;
			return sent;
		}

		if( !rv )
		{
			// wait another 20msec and try again
			if( tries-- > 0 )
				continue;

			// we cannot write just yet
			return sent;
		}

		if( ( wr = send( s->sock, ptr, len, MSG_NOSIGNAL ) ) < 0 )
		{
			warn( "Error writing to host %s -- %s",
				s->name, Err );
			s->flags |= HOST_CLOSE;
			return sent;
		}

		len  -= wr;
		ptr  += wr;
		sent += wr;
	}

	// weirdness
	if( len >= b->len )
		b->len = 0;

	//debug( "Wrote to %d bytes to %d/%s", sent, s->sock, s->name );

	// what we wrote
	return sent;
}


int io_connected( NSOCK *s )
{
	socklen_t len;
	int err;

	if( s->sock < 0 )
		return -1;

	err = 0;
	len = sizeof( int );
	
	if( getsockopt( s->sock, SOL_SOCKET, SO_ERROR, &err, &len ) < 0 )
	{
		warn( "Could not assess target socket state: %s", Err );
		close( s->sock );
		s->sock = -1;
		return -1;
	}

	if( err != 0 )
	{
		warn( "Target socket errored: %s", strerror( err ) );
		close( s->sock );
		s->sock = -1;
		return -1;
	}

	return 0;
}



int io_connect( NSOCK *s )
{
	int opt = 1;
	char *label;

	if( s->sock != -1 )
	{
		warn( "Net connect called on connected socket - disconnecting." );
		shutdown( s->sock, SHUT_RDWR );
		close( s->sock );
		s->sock = -1;
	}

	if( !s->name || !*(s->name) )
		label = "unknown socket";
	else
		label = s->name;

	if( ( s->sock = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
	{
		err( "Unable to make tcp socket for %s -- %s",
			label, Err );
		return -1;
	}

	if( setsockopt( s->sock, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof( int ) ) )
	{
		err( "Unable to set keepalive on socket for %s -- %s",
			label, Err );
		close( s->sock );
		s->sock = -1;
		return -1;
	}

	if( connect( s->sock, (struct sockaddr *) s->peer, sizeof( struct sockaddr_in ) ) < 0 )
	{
		err( "Unable to connect to %s:%hu for %s -- %s",
			inet_ntoa( s->peer->sin_addr ), ntohs( s->peer->sin_port ),
			label, Err );
		close( s->sock );
		s->sock = -1;
		return -1;
	}

	info( "Connected (%d) to remote host %s:%hu for %s.",
		s->sock, inet_ntoa( s->peer->sin_addr ),
		ntohs( s->peer->sin_port ), label );

	s->flags = 0;

	return s->sock;
}






// attach a buffer for sending
void io_buf_send( IOBUF *buf )
{
	int freebuf = 0;

	if( !buf )
		return;

	// it does contain something, right?
	if( !buf->len )
	{
		warn( "Empty buffer passed to io_buf_send." );
		mem_free_buf( &buf );
		return;
	}

	pthread_mutex_lock( &(ctl->locks->iobuffers) );

	if( ctl->net->target->bufs >= ctl->net->max_bufs )
		freebuf = 1;
	else
	{
		buf->next = ctl->net->target->in;
		ctl->net->target->in = buf;
		ctl->net->target->bufs++;
	}

	pthread_mutex_unlock( &(ctl->locks->iobuffers) );

	if( freebuf )
	{
		debug( "Throwing away buffer - too much waiting already." );
		mem_free_buf( &buf );
	}
}


void io_grab_buffer( NSOCK *s )
{
	IOBUF *b, *prev;

	// this is safe here because this thread is the only
	// one that takes things away from this pointer
	if( !s->in )
		return;

	pthread_mutex_lock( &(ctl->locks->iobuffers) );

	// remove the last buffer
	for( prev = NULL, b = s->in; b->next; prev = b, b = b->next );

	if( prev )
		prev->next = NULL;
	else
		s->in = NULL;

	// and decrement the buffers
	s->bufs--;
	if( s->bufs < 0 )
	{
		warn( "Target socket buffers went below 0." );
		s->bufs = 0;
	}

	pthread_mutex_unlock( &(ctl->locks->iobuffers) );

	// and attach it to out
	s->out = b;
}



void io_send( NSOCK *s )
{
	int l, rv, i;

#ifdef DEBUG
	debug( "IO Send" );
#endif

	if( !s->out )
		io_grab_buffer( s );

	i = 0;

	while( s->out )
	{
		i++;

		// try to send the out buffer
		l = s->out->len;
		rv = io_write_data( s );

		// did we sent it all?

		// did we have problems?
		if( s->flags & HOST_CLOSE )
		{
			net_disconnect( &(s->sock), "send target" );
			s->flags &= ~HOST_CLOSE;
			break;
		}

		if( rv == l )
		{
			// free that buffer
			mem_free_buf( &(s->out) );

			// and get another
			io_grab_buffer( s );
		}
		else
			// try again later
			break;
	}
}


void *io_loop( void *arg )
{
	struct sockaddr_in sa, *sp;
	struct addrinfo *ai;
	NSOCK *s;
	THRD *t;

	t = (THRD *) arg;

	// find out where we are connecting to
	if( getaddrinfo( ctl->net->host, NULL, NULL, &ai ) || !ai )
	{
		err( "Could not look up target host %s -- %s", ctl->net->host, Err );
		loop_end( "Unable to loop up network target." );
		free( t );
		return NULL;
	}

	sp = (struct sockaddr_in *) ai->ai_addr;

	// we'll take the first address thanks
	sa.sin_family = sp->sin_family;
	sa.sin_addr   = sp->sin_addr;
	// and we already have a port
	sa.sin_port   = htons( ctl->net->port );

	// done with that
	freeaddrinfo( ai );

	// make a socket
	s = net_make_sock( 0, 0, "io target", &sa );
	ctl->net->target = s;

	// say we've started
	loop_mark_start( "io" );

	// now loop around sending
	while( ctl->run_flags & RUN_LOOP )
	{
		// keep trying if we are not connected
		if( io_connected( s ) < 0
		 && io_connect( s ) < 0 )
		{
			// don't try to reconnect at once
			// sleep a few seconds
			usleep( ctl->net->reconn );
			continue;
		}

		// right, we're good
		io_send( s );

		// and sleep a little
		usleep( ctl->net->io_usec );
	}

	loop_mark_done( "io" );

	// shut down that socket.  Never mind freeing stuff,
	// we are exiting.
	if( s->sock > -1 )
	{
		shutdown( s->sock, SHUT_RDWR );
		close( s->sock );
		s->sock = -1;
	}

	free( t );
	return NULL;
}


