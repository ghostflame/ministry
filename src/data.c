/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* data.c - handles connections and processes stats lines                  *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "ministry.h"


const DTYPE data_type_defns[DATA_TYPE_MAX] =
{
	{
		.type = DATA_TYPE_STATS,
		.name = "stats",
		.lf   = &data_line_ministry,
		.pf   = &data_line_min_prefix,
		.af   = &data_point_stats,
		.port = DEFAULT_STATS_PORT,
		.lock = DSTATS_SLOCK_COUNT,
		.sock = "ministry stats socket"
	},
	{
		.type = DATA_TYPE_ADDER,
		.name = "adder",
		.lf   = &data_line_ministry,
		.pf   = &data_line_min_prefix,
		.af   = &data_point_adder,
		.port = DEFAULT_ADDER_PORT,
		.lock = DADDER_SLOCK_COUNT,
		.sock = "ministry adder socket"
	},
	{
		.type = DATA_TYPE_GAUGE,
		.name = "gauge",
		.lf   = &data_line_ministry,
		.pf   = &data_line_min_prefix,
		.af   = &data_point_gauge,
		.port = DEFAULT_GAUGE_PORT,
		.lock = DGAUGE_SLOCK_COUNT,
		.sock = "ministry gauge socket"
	},
	{
		.type = DATA_TYPE_COMPAT,
		.name = "compat",
		.lf   = &data_line_compat,
		.pf   = &data_line_com_prefix,
		.af   = NULL,
		.port = DEFAULT_COMPAT_PORT,
		.sock = "statsd compat socket"
	},
};



static const uint32_t data_cksum_primes[8] =
{
	2909, 3001, 3083, 3187, 3259, 3343, 3517, 3581
};

__attribute__((hot)) static inline uint32_t data_path_cksum( char *str, int len )
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



__attribute__((hot)) static inline DHASH *data_find_path( DHASH *list, uint32_t hval, char *path, int len )
{
	DHASH *h;

	for( h = list; h; h = h->next )
		if( h->sum == hval
		 && h->len == len	// h->len is set to 0 by gc
		 && !memcmp( h->path, path, len ) )
			break;

	return h;
}



DHASH *data_locate( char *path, int len, int type )
{
	uint32_t hval;
	ST_CFG *c;
	DHASH *d;

	hval = data_path_cksum( path, len );

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
	uint32_t hval, idx;
	DHASH *d;

	hval = data_path_cksum( path, len );
	idx  = hval % c->hsize;

	if( !( d = data_find_path( c->data[idx], hval, path, len ) ) )
	{
		lock_table( idx );

		if( !( d = data_find_path( c->data[idx], hval, path, len ) ) )
		{
			if( !( d = mem_new_dhash( path, len, c->dtype ) ) )
			{
				fatal( "Could not allocate dhash for %s.", c->name );
				return NULL;
			}

			d->sum = hval;

			d->next = c->data[idx];
			c->data[idx] = d;

			d->valid = 1;
		}

		unlock_table( idx );

		d->id = data_get_id( c );
	}

	return d;
}





void data_point_gauge( char *path, int len, char *dat )
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

	val  = strtod( dat, NULL );

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
	(*(h->type->handler))( h->workbuf, plen, ep );
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
	(*(h->type->handler))( line, plen, ep );
}




// parse the lines
// put any partial lines back at the start of the buffer
// and return the length, if any
__attribute__((hot)) int data_parse_buf( HOST *h, char *buf, int len )
{
	register char *s = buf;
	register char *q;
	char *r;
	int l;

	// can't parse without a handler function
	// and those live on the host object
	if( !h )
		return 0;

	while( len > 0 )
	{
		// look for newlines
		if( !( q = memchr( s, LINE_SEPARATOR, len ) ) )
		{
			// partial last line
			l = s - buf;

			if( len < l )
			{
				memcpy( buf, s, len );
				*(buf + len) = '\0';
			}
			else
			{
				q = buf;
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
			*r-- = '\0';
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

	return len;
}




