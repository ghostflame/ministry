/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* data.c - handles connections and processes stats lines                  *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "ministry.h"


DTYPE data_type_defns[DATA_TYPE_MAX] =
{
	{
		.type = DATA_TYPE_STATS,
		.name = "stats",
		.lf   = &data_line_ministry,
		.pf   = &data_line_min_prefix,
		.af   = &data_point_stats,
		.tokn = TOKEN_TYPE_STATS,
		.port = DEFAULT_STATS_PORT,
		.thrd = TCP_THRD_DSTATS,
		.sock = "ministry stats socket",
		.nt   = NULL
	},
	{
		.type = DATA_TYPE_ADDER,
		.name = "adder",
		.lf   = &data_line_ministry,
		.pf   = &data_line_min_prefix,
		.af   = &data_point_adder,
		.tokn = TOKEN_TYPE_ADDER,
		.port = DEFAULT_ADDER_PORT,
		.thrd = TCP_THRD_DADDER,
		.sock = "ministry adder socket",
		.nt   = NULL
	},
	{
		.type = DATA_TYPE_GAUGE,
		.name = "gauge",
		.lf   = &data_line_ministry,
		.pf   = &data_line_min_prefix,
		.af   = &data_point_gauge,
		.tokn = TOKEN_TYPE_GAUGE,
		.port = DEFAULT_GAUGE_PORT,
		.thrd = TCP_THRD_DGAUGE,
		.sock = "ministry gauge socket",
		.nt   = NULL
	},
	{
		.type = DATA_TYPE_COMPAT,
		.name = "compat",
		.lf   = &data_line_compat,
		.pf   = &data_line_com_prefix,
		.af   = NULL,
		.tokn = 0,
		.port = DEFAULT_COMPAT_PORT,
		.thrd = TCP_THRD_DCOMPAT,
		.sock = "statsd compat socket",
		.nt   = NULL
	},
};



static const uint64_t data_path_hash_primes[8] =
{
	2909, 3001, 3083, 3187, 3259, 3343, 3517, 3581
};

/*
 * This is loosely based on DJ Bernsteins per-character hash, but with
 * significant speedup from the 32-bit in pointer cast.
 *
 * It replaces an xor based hash that showed too many collisions.
 */
__attribute__((hot)) static inline uint64_t data_path_hash( char *str, int len )
{
	register uint64_t sum = 5381;
	register int ctr, rem;
	register uint32_t *p;

	rem = len & 0x3;
	ctr = len >> 2;
	p   = (uint32_t *) str;

	/*
	 * Performance
	 *
	 * I've tried this with unrolled loops of 2, 4 and 8
	 *
	 * Loops of 8 don't happen enough - the strings aren't
	 * long enough.
	 *
	 * Loops of 4 work very well.
	 *
	 * Loops of 2 aren't enough improvement.
	 *
	 * Strangely, loops of 8 then 4 then 2 then 1 are slower
	 * than just 4 then 1, so this is pretty optimized as is.
	 */

	// a little unrolling for good measure
	while( ctr > 4 )
	{
		sum += ( sum << 5 ) + *p++;
		sum += ( sum << 5 ) + *p++;
		sum += ( sum << 5 ) + *p++;
		sum += ( sum << 5 ) + *p++;
		ctr -= 4;
	}

	// and the rest
	while( ctr-- > 0 )
		sum += ( sum << 5 ) + *p++;

	// and capture the rest
	str = (char *) p;
	while( rem-- > 0 )
		sum += *str++ * data_path_hash_primes[rem];

	return sum;
}


uint64_t data_path_hash_wrap( char *str, int len )
{
	return data_path_hash( str, len );
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



__attribute__((hot)) static inline DHASH *data_find_path( DHASH *list, uint64_t hval, char *path, int len )
{
	register DHASH *h;

	for( h = list; h; h = h->next )
		if( h->sum == hval
		 && h->valid
		 && h->len == len
		 && !memcmp( h->path, path, len ) )
			break;

	return h;
}



DHASH *data_locate( char *path, int len, int type )
{
	uint64_t hval;
	ST_CFG *c;
	DHASH *d;

	hval = data_path_hash( path, len );

	switch( type )
	{
		case DATA_TYPE_STATS:
			c = ctl->stats->stats;
			break;
		case DATA_TYPE_ADDER:
			c = ctl->stats->adder;
			break;
		case DATA_TYPE_GAUGE:
			c = ctl->stats->gauge;
			break;
		default:
			// try each in turn
			if( ( d = data_locate( path, len, DATA_TYPE_STATS ) ) )
				return d;
			if( ( d = data_locate( path, len, DATA_TYPE_ADDER ) ) )
				return d;
			if( ( d = data_locate( path, len, DATA_TYPE_GAUGE ) ) )
				return d;
			return NULL;
	}

	return data_find_path( c->data[hval % c->hsize], hval, path, len );
}



__attribute__((hot)) static inline DHASH *data_get_dhash( char *path, int len, ST_CFG *c )
{
	uint64_t hval, idx, crt;
	DHASH *d;

	hval = data_path_hash( path, len );
	idx  = hval % c->hsize;
	crt  = 0;

	if( !( d = data_find_path( c->data[idx], hval, path, len ) ) )
	{
		lock_table( idx );

		if( !( d = data_find_path( c->data[idx], hval, path, len ) ) )
		{
			if( !( d = mem_new_dhash( path, len ) ) )
			{
				fatal( "Could not allocate dhash for %s.", c->name );
				return NULL;
			}

			d->sum  = hval;
			d->type = c->dtype;
			d->next = c->data[idx];
			c->data[idx] = d;

			d->valid = 1;

			// mark this newly created
			crt = 1;
		}

		unlock_table( idx );

		if( crt )
		{
			d->id = data_get_id( c );

			// might we do moment filtering on this?
			if( c->dtype == DATA_TYPE_STATS && ctl->stats->mom->enabled )
			{
				if( regex_list_test( d->path, ctl->stats->mom->rgx ) == 0 )
				{
				//	debug( "Path %s will get moments processing.", d->path );
					d->mom_check = 1;
				}
				//else
				//	debug( "Path %s will not get moments processing.", d->path );
			}
			else if( c->dtype == DATA_TYPE_ADDER && ctl->stats->pred->enabled )
			{
				if( regex_list_test( d->path, ctl->stats->pred->rgx ) == 0 )
				{
				//	debug( "Path %s will get linear regression prediction.", d->path );
					d->predict = mem_new_pred( );
				}
				//else
				//	debug( "Path %s will not get prediction.", d->path );
			}
		}
	}

	return d;
}





__attribute__((hot)) void data_point_gauge( char *path, int len, char *dat )
{
	double val;
	DHASH *d;
	char op;

	// gauges can have relative changes
	if( *dat == '+' || *dat == '-' )
		op = *dat++;
	else
		op = '\0';

	// which means to explicitly set a gauge to a negative
	// number, you must first set it to zero.  Don't blame me,
	// this follows the statsd guide
	// https://github.com/etsy/statsd/blob/master/docs/metric_types.md
	val = strtod( dat, NULL );

	d = data_get_dhash( path, len, ctl->stats->gauge );

	// lock that path
	lock_gauge( d );

	// add in the data, based on the op
	// add, subtract or set
	switch( op )
	{
		case '+':
			d->in.total += val;
			break;
		case '-':
			d->in.total -= val;
			break;
		default:
			d->in.total = val;
			break;
	}

	++(d->in.count);

	// and unlock
	unlock_gauge( d );
}



__attribute__((hot)) void data_point_adder( char *path, int len, char *dat )
{
	double val;
	DHASH *d;

	val = strtod( dat, NULL );

	d = data_get_dhash( path, len, ctl->stats->adder );

	// lock that path
	lock_adder( d );

	// add in that data point
	d->in.total += val;
	++(d->in.count);

	// and unlock
	unlock_adder( d );
}


__attribute__((hot)) void data_point_stats( char *path, int len, char *dat )
{
	double val;
	PTLIST *p;
	DHASH *d;

	val = strtod( dat, NULL );

	d = data_get_dhash( path, len, ctl->stats->stats );

	// lock that path
	lock_stats( d );

	// make a new one if need be
	if( !( p = d->in.points ) || p->count >= PTLIST_SIZE )
	{
		if( !( p = mem_new_point( ) ) )
		{
			fatal( "Could not allocate new point struct." );
			unlock_stats( d );
			return;
		}

		p->next = d->in.points;
		d->in.points = p;
	}

	// keep that data point
	p->vals[p->count] = val;
	++(d->in.count);
	++(p->count);

	// and unlock
	unlock_stats( d );
}


// break up ministry type line
__attribute__((hot)) static inline int __data_line_ministry_check( char *line, int len, char **end )
{
	register char *sp;
	int plen;

	if( !( sp = memchr( line, FIELD_SEPARATOR, len ) ) )
	{
		// allow keepalive lines
		if( len == 9 && !memcmp( line, "keepalive", 9 ) )
			return 0;
		return -1;
	}

	plen  = sp - line;
	*sp++ = '\0';

	if( !plen || !*sp )
		return -1;

	*end = sp;
	return plen;
}



// break up a statsd type line
__attribute__((hot)) static inline int __data_line_compat_check( char *line, int len, char **dat, char **tp )
{
	register char *cl;
	char *vb;
	int plen;

	if( !( cl = memchr( line, ':', len ) ) )
	{
		// allow keepalive lines
		if( len == 9 && !memcmp( line, "keepalive", 9 ) )
			return 0;
		return -1;
	}

	plen  = cl - line;
	*cl++ = '\0';

	if( !plen || !*cl )
		return -1;

	*dat = cl;
	len -= plen + 1;

	if( !( vb = memchr( cl, '|', len ) ) )
		return -1;

	*vb++ = '\0';
	*tp   = vb;

	return plen;
}


// dispatch a statsd line based on type
__attribute__((hot)) static inline int __data_line_compat_dispatch( char *path, int len, char *data, char type )
{
	switch( type )
	{
		case 'c':
			data_point_adder( path, len, data );
			break;
		case 'm':
			data_point_stats( path, len, data );
			break;
		case 'g':
			data_point_gauge( path, len, data );
			break;
		default:
			return -1;
	}

	return 0;
}



// support the statsd format but adding a prefix
// path:<val>|<c or ms>
__attribute__((hot)) void data_line_com_prefix( HOST *h, char *line, int len )
{
	char *data = NULL, *type = NULL;
	int plen;

	if( ( plen = __data_line_compat_check( line, len, &data, &type ) ) < 0 || plen > h->lmax )
	{
		h->invalid++;
		return;
	}

	if( !plen )
		return;  // probably a keepalive

	// copy the line next to the prefix
	memcpy( h->ltarget, line, plen );
	plen += h->plen;
	h->ltarget[plen] = '\0';

	if( __data_line_compat_dispatch( h->workbuf, plen, data, *type ) < 0 )
		h->invalid++;
	else
		h->points++;
}



// support the statsd format
// path:<val>|<c or ms>
__attribute__((hot)) void data_line_compat( HOST *h, char *line, int len )
{
	char *data = NULL, *type = NULL;
	int plen;

	if( ( plen = __data_line_compat_check( line, len, &data, &type ) ) < 0 )
	{
		h->invalid++;
		return;
	}

	if( !plen )
		return;  // probably a keepalive

	if( __data_line_compat_dispatch( line, plen, data, *type ) < 0 )
		h->invalid++;
	else
		h->points++;
}



__attribute__((hot)) void data_line_min_prefix( HOST *h, char *line, int len )
{
	char *ep = NULL;
	int plen;

	if( ( plen = __data_line_ministry_check( line, len, &ep ) ) < 0 || plen > h->lmax )
	{
		h->invalid++;
		return;
	}

	if( !plen )
		return;  // probably a keepalive

	// looks OK
	h->points++;

	// copy that into place
	memcpy( h->workbuf + h->plen, line, plen );
	plen += h->plen;
	h->workbuf[plen] = '\0';

	// and deal with it
	(*(h->handler))( h->workbuf, plen, ep );
}




__attribute__((hot)) void data_line_ministry( HOST *h, char *line, int len )
{
	char *ep = NULL;
	int plen;

	if( ( plen = __data_line_ministry_check( line, len, &ep ) ) < 0 )
	{
		h->invalid++;
		return;
	}

	if( !plen )
		return;  // probably a keepalive

	// looks OK
	h->points++;

	// and put that in
	(*(h->handler))( line, plen, ep );
}




__attribute__((hot)) void data_line_token( HOST *h, char *line, int len )
{
	int64_t tval;
	TOKEN *t;

	// have we flagged them already?
	if( h->net->flags & IO_CLOSE )
	{
		debug( "Host already flagged as closed - ignoring line: %s", line );
		return;
	}

	// read the token
	tval = strtoll( line, NULL, 10 );

	// look up a token based on that information
	if( !( t = token_find( h->ip, h->type->token_type, tval ) ) )
	{
		debug( "Found no token: %p", t );

		// we were expecting a token, so, no.
		h->net->flags |= IO_CLOSE;
		return;
	}

	// burn that token
	token_burn( t );

	// reset the handler function
	net_set_host_parser( h, 0, 1 );
}




// parse the lines
// put any partial lines back at the start of the buffer
// and return the length, if any
__attribute__((hot)) void data_parse_buf( HOST *h, IOBUF *b )
{
	register char *s = b->buf;
	register char *q;
	int len, l;
	char *r;

	// can't parse without a handler function
	// and those live on the host object
	if( !h )
		return;

	len = b->len;

	while( len > 0 )
	{
		// look for newlines
		if( !( q = memchr( s, LINE_SEPARATOR, len ) ) )
		{
			// partial last line
			l = s - b->buf;

			if( len < l )
			{
				memcpy( b->buf, s, len );
				*(b->buf + len) = '\0';
			}
			else
			{
				q = b->buf;
				l = len;

				while( l-- > 0 )
					*q++ = *s++;
				*q = '\0';
			}

			// and we're done, with len > 0
			break;
		}

		l = q - s;
		r = q - 1;

		// stomp on that newline
		*q++ = '\0';

		// and decrement the remaining length
		len -= q - s;

		// clean leading \r's
		if( *s == '\r' )
		{
			s++;
			l--;
		}

		// get the length
		// and trailing \r's
		if( l > 0 && *r == '\r' )
		{
			*r = '\0';
			l--;
		}

		// still got anything?
		if( l > 0 )
		{
			// process that line
			(*(h->parser))( h, s, l );
		}

		// and move on
		s = q;
	}

	// and update the buffer length
	b->len = len;
}


void data_fetch_cb( void *arg, IOBUF *b )
{
	FETCH *f = (FETCH *) arg;

	data_parse_buf( f->host, b );
}

