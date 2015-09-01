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
#ifdef CKSUM_64BIT
	register uint64_t *p, sum = 0xbeef;
#else
	register uint32_t *p, sum = 0xbeef;
#endif
	int rem;

#ifdef CKSUM_64BIT
	rem = len & 0x7;
	len = len >> 3;

	for( p = (uint64_t *) str; len > 0; len-- )
#else
	rem = len & 0x3;
	len = len >> 2;

	for( p = (uint32_t *) str; len > 0; len-- )
#endif
		sum ^= *p++;

	// and capture the rest
	str = (char *) p;
	while( rem-- > 0 )
		sum += *str++ * data_cksum_primes[rem];

#ifdef CKSUM_64BIT
	sum = ( ( sum >> 32 ) ^ ( sum & 0xffffffff ) ) & 0xffffffff;
#endif

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


void data_point_adder( char *path, int len, unsigned long long val )
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
	d->in.total += val;

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
				strtoull( h->val->wd[1], NULL, 10 ) );
}




void data_handle_connection( HOST *h )
{
	struct pollfd p;
	int rv, i;

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

		// 0 means gone away
		if( ( rv = io_read_lines( h ) ) <= 0 )
		{
			debug( "Received 0 lines from %s -- gone away?", h->net->name );
			h->net->flags |= HOST_CLOSE;
			break;
		}

		// mark them active
		h->last = ctl->curr_time.tv_sec;

		// and handle them
		for( i = 0; i < h->all->wc; i++ )
			if( h->all->len[i] > 0 )
				(*(h->type->handler))( h, h->all->wd[i], h->all->len[i] );

		// allow data fetch to tell us to close this host down
		if( h->net->flags & HOST_CLOSE )
		{
			debug( "Host %s flagged as closed.", h->net->name );
			break;
		}
	}
}



void *data_connection( void *arg )
{
	THRD *t;
	HOST *h;

	t = (THRD *) arg;
	h = (HOST *) t->arg;

	info( "Accepted data connection from host %s", h->net->name );

	// make sure we can be cancelled
	pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, NULL );
	pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, NULL );

	data_handle_connection( h );

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
			h = net_get_host( p.fd, n->type );
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




