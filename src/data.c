/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* data.c - handles connections and processes stats lines                  *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "ministry.h"


static uint32_t data_cksum_primes[8] =
{
	2909, 3001, 3083, 3187, 3259, 3343, 3517, 3581
};

uint32_t data_path_cksum( char *str, int len )
{
	register uint32_t *p, sum = 0xbeef;
	int rem;

	rem = len & 0x3;
	len = len >> 2;
	p   = (uint32_t *) str;

	// a little unrolling for good measure
	while( len > 4 )
	{
		sum ^= *p++;
		sum ^= *p++;
		sum ^= *p++;
		sum ^= *p++;
		len -= 4;
	}

	// and the rest
	while( len-- > 0 )
		sum ^= *p++;

	// and capture the rest
	str = (char *) p;
	while( rem-- > 0 )
		sum += *str++ * data_cksum_primes[rem];

	return sum;
}



uint32_t data_get_id( ST_CFG *st )
{
	uint32_t id;

	pthread_mutex_lock( &(ctl->locks->hashstats) );

	// grab an id - these never drop
	id = ++(st->did);

	// and keep stats - gc reduces this
	st->dcurr++;

	pthread_mutex_unlock( &(ctl->locks->hashstats) );

	return id;
}



inline DHASH *data_find_path( DHASH *list, uint32_t hval, char *path, int len )
{
	DHASH *h;

	for( h = list; h; h = h->next )
		if( h->sum == hval
		 && h->len == len
		 && !memcmp( h->path, path, len ) )
			break;

	return h;
}

#define data_find_stats( idx, h, p, l )		data_find_path( ctl->stats->stats->data[idx], h, p, l )
#define data_find_adder( idx, h, p, l )		data_find_path( ctl->stats->adder->data[idx], h, p, l )


DHASH *data_locate( char *path, int len, int adder )
{
	uint32_t hval, indx;

	hval = data_path_cksum( path, len );
	indx = hval % ctl->mem->hashsize;

	return ( adder == DATA_HTYPE_ADDER ) ?
	       data_find_adder( indx, hval, path, len ) :
	       data_find_stats( indx, hval, path, len );
}



void data_point_adder( char *path, int len, double val )
{
	uint32_t hval, indx;
	DHASH *d;

	hval = data_path_cksum( path, len );
	indx = hval % ctl->mem->hashsize;

	if( !( d = data_find_adder( indx, hval, path, len ) ) )
	{
		lock_table( indx );

		if( !( d = data_find_adder( indx, hval, path, len ) ) )
		{
			d = mem_new_dhash( path, len, DATA_HTYPE_ADDER );
			d->sum = hval;

#ifdef DEBUG
			debug( "Added new adder path (%u: %u) %s", indx, hval, path );
#endif

			d->next = ctl->stats->adder->data[indx];
			ctl->stats->adder->data[indx] = d;
		}

		unlock_table( indx );

		// and grab an ID for it
		d->id = data_get_id( ctl->stats->adder );
	}

	// lock that path
	lock_adder( d );

	// add in that data point
	d->in.sum.total += val;
	d->in.sum.count++;

	// and unlock
	unlock_adder( d );
}


void data_point_stats( char *path, int len, float val )
{
	uint32_t hval, indx;
	PTLIST *p;
	DHASH *d;

	hval = data_path_cksum( path, len );
	indx = hval % ctl->mem->hashsize;

	// there is a theoretical race condition here
	// so we check again in a moment under lock
	if( !( d = data_find_stats( indx, hval, path, len ) ) )
	{
		lock_table( indx );

		if( !( d = data_find_stats( indx, hval, path, len ) ) )
		{
			d = mem_new_dhash( path, len, DATA_HTYPE_STATS );
			d->sum = hval;

			d->next = ctl->stats->stats->data[indx];
			ctl->stats->stats->data[indx] = d;
		}

		unlock_table( indx );

		// and grab an ID for it
		d->id = data_get_id( ctl->stats->stats );
	}

	// lock that path
	lock_stats( d );

	// find where to store the points
	for( p = d->in.points; p; p = p->next )
		if( p->count < PTLIST_SIZE )
			break;

	// make a new one if need be
	if( !p )
	{
		p = mem_new_point( );
		p->next = d->in.points;
		d->in.points = p;
	}

	// keep that data point
	p->vals[p->count++] = val;

	// and unlock
	unlock_stats( d );
}


// support the statsd format
// path:<val>|<c or ms>
void data_line_statsd( HOST *h, char *line, int len )
{
	char *cl, *vb;
	int plen;

	if( !( cl = memchr( line, ':', len ) ) )
	{
		h->invalid++;
		return;
	}

	// stomp on that
	*cl  = '\0';
	plen = cl - line;

	cl++;
	len -= plen + 1;

	if( !( vb = memchr( cl, '|', len ) ) )
	{
		h->invalid++;
		return;
	}

	// and stomp on that
	*vb++ = '\0';

	switch( *vb )
	{
		case 'c':
			// counter - integer
			data_point_adder( line, plen, strtoull( cl, NULL, 10 ) );
			h->points++;
			break;
		case 'm':
			// msec - double
			data_point_stats( line, plen, strtod( cl, NULL ) );
			h->points++;
			break;
		default:
			h->invalid++;
			break;
	}
}


void data_line_data( HOST *h, char *line, int len )
{
	// break that up
	strwords( h->val, line, len, FIELD_SEPARATOR );

	// broken?
	if( h->val->wc < STAT_FIELD_MIN )
	{
		h->invalid++;
		return;
	}

	// looks OK
	h->points++;

	data_point_stats( h->val->wd[0], h->val->len[0],
	                 strtod( h->val->wd[1], NULL ) );
}


void data_line_adder( HOST *h, char *line, int len )
{
	// break it up
	strwords( h->val, line, len, FIELD_SEPARATOR );

	// broken?
	if( h->val->wc < STAT_FIELD_MIN )
	{
		h->invalid++;
		return;
	}

	// looks ok
	h->points++;

	// and put that in
	data_point_adder( h->val->wd[0], h->val->len[0],
				strtod( h->val->wd[1], NULL ) );
}



//  This function is an annoying mix of io code and processing code,
//  but it is like that to address the following concerns:
//
//  1.  Data may spill across multiple receives, and not on line breaks
//  2.  We may receive more data than fits in one strwords struct in one go
//
//  So we have to keep looping around read, looking for more data until we
//  don't get any.  Then we need to loop around processing until there's no
//  data left.  We need to only barf on no data the first time around, thus
//  the empty flag.  We need to keep any partial lines for the next read call
//  to push back to the start of the buffer.
//
int data_recv_lines( HOST *h )
{
	int i, l, keeplast = 0, len;
	NSOCK *n = h->net;
	char *src, *w;

	n->flags |= HOST_CLOSE_EMPTY;

	// we need to loop until there's nothing left to read
	for( ; ; )
	{
		// try to read some data
		if( ( i = io_read_data( h->net ) ) <= 0 )
			break;

		// do we have anything
		if( !n->in->len )
		{
			debug( "No incoming data from %s", n->name );
			break;
		}

		// remove the close-empty flag
		// we only want to close if our first
		// read finds nothing
		n->flags &= ~HOST_CLOSE_EMPTY;

		// is the last line complete?
		if( n->in->buf[n->in->len - 1] == LINE_SEPARATOR )
			// remove any trailing separator
			n->in->buf[--(n->in->len)] = '\0';
		else
			// make a note to keep the last line back
			keeplast = 1;


		// we may have to go around the breakup/handle
		// loop multiple times if we get more lines than
		// strwords can handle

		src = n->in->buf;
		len = n->in->len;

		while( src && len > 0 )
		{
			if( strwords( h->all, src, len, LINE_SEPARATOR ) < 0 )
			{
				debug( "Invalid buffer from data host %s.", n->name );
				return 0;
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

			// note any remainder
			src = h->all->end;
			len = h->all->end_len;

			// let's process those lines
			for( i = 0; i < h->all->wc; i++ )
				if( h->all->len[i] > 0 )
					(*(h->type->handler))( h, h->all->wd[i], h->all->len[i] );

			// if the last line was incomplete, don't process it
			// but mark up the keep buffer
			if( !len && h->all->wc && keeplast )
			{
				h->all->wc--;
				n->keep->buf = h->all->wd[h->all->wc];
				n->keep->len = h->all->len[h->all->wc];
			}
		}
	}

	// did we get something?  or are we done?
	if( n->flags & HOST_CLOSE )
	{
		debug( "Host %s flagged as closed.", n->name );
		return -1;
	}
	else
		h->last = ctl->curr_time.tv_sec;

	return 0;
}





void *data_connection( void *arg )
{
	struct pollfd p;
	int rv;

	THRD *t;
	HOST *h;

	t = (THRD *) arg;
	h = (HOST *) t->arg;

	info( "Accepted data connection from host %s", h->net->name );

	// make sure we can be cancelled
	pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, NULL );
	pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, NULL );

	p.fd     = h->net->sock;
	p.events = POLL_EVENTS;

	while( ctl->run_flags & RUN_LOOP )
	{
		if( ( rv = poll( &p, 1, 500 ) ) < 0 )
		{
			// don't sweat interruptions
			if( errno == EINTR )
			{
				debug( "Poll call was interrupted." );
				continue;
			}

			warn( "Poll error talk to host %s -- %s",
				h->net->name, Err );
			break;
		}
		if( !rv )
			continue;

		// they went away?
		if( p.revents & POLLHUP )
		{
			debug( "Received pollhup event from %s", h->net->name );
			break;
		}

		// and process the lines
		if( data_recv_lines( h ) < 0 )
			break;
	}


	info( "Closing connection to host %s after %lu data points.",
			h->net->name, h->points );

	net_close_host( h );

	free( t );
	return NULL;
}



void *data_loop_udp( void *arg )
{
	socklen_t sl;
	NET_PORT *n;
	IOBUF *b;
	THRD *t;
	HOST *h;
	int i;

	t = (THRD *) arg;
	n = (NET_PORT *) t->arg;
	h = mem_new_host( );
	b = h->net->in;

	h->type = n->type;

	loop_mark_start( "udp" );

	while( ctl->run_flags & RUN_LOOP )
	{
		// get a packet, set the from
		sl = sizeof( struct sockaddr_in );
		if( ( b->len = recvfrom( n->sock, b->buf, b->sz, 0,
						(struct sockaddr *) &(h->peer), &sl ) ) < 0 )
		{
			if( errno == EINTR || errno == EAGAIN )
				continue;

			err( "Recvfrom error -- %s", Err );
			loop_end( "receive error on udp socket" );
			break;
		}
		else if( !b->len )
			continue;

		if( b->len < b->sz )
			b->buf[b->len] = '\0';

		// break that up
		//
		// we don't have a way to handle partial lines received over
		// UDP without keeping a cache of keep buffers for every IP address
		// seen.  nc will certainly break lines across packets and if your
		// app does the same, we will likely screw up your data.  But that's
		// UDP for you.
		//
		// An IP based cache is a DoS vulnerability - we could be supplied
		// with partial lines from a huge variety of source addresses and
		// balloon our memory trying to keep up.  So for now, a simpler
		// view is necessary
		if( !strwords( h->all, (char *) b->buf, b->len, LINE_SEPARATOR ) )
			continue;

		// and handle them
		for( i = 0; i < h->all->wc; i++ )
			if( h->all->len[i] > 0 )
				(*(h->type->handler))( h, h->all->wd[i], h->all->len[i] );
	}

	loop_mark_done( "udp" );

	mem_free_host( &h );

	free( t );
	return NULL;
}




void *data_loop_tcp( void *arg )
{
	struct pollfd p;
	NET_PORT *n;
	THRD *t;
	HOST *h;
	int rv;

	t = (THRD *) arg;
	n = (NET_PORT *) t->arg;

	p.fd     = n->sock;
	p.events = POLL_EVENTS;

	loop_mark_start( "tcp" );

	while( ctl->run_flags & RUN_LOOP )
	{
		if( ( rv = poll( &p, 1, 500 ) ) < 0 )
		{
			if( errno != EINTR )
			{
				err( "Poll error on %s -- %s", n->type->label, Err );
				loop_end( "polling error on tcp socket" );
				break;
			}
		}
		else if( !rv )
			continue;

		// go get that then
		if( p.revents & POLL_EVENTS )
		{
			if( ( h = net_get_host( p.fd, n->type ) ) )
				thread_throw_watched( data_connection, h );
		}
	}

	loop_mark_done( "tcp" );

	free( t );
	return NULL;
}



void data_start( NET_TYPE *nt )
{
	int i;

	if( !( nt->flags & NTYPE_ENABLED ) )
		return;

	if( nt->flags & NTYPE_TCP_ENABLED )
		thread_throw( data_loop_tcp, nt->tcp );

	if( nt->flags & NTYPE_UDP_ENABLED )
		for( i = 0; i < nt->udp_count; i++ )
			thread_throw( data_loop_udp, nt->udp[i] );

	info( "Started listening for data on %s", nt->label );
}




