/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* metrics.c - functions for prometheus-style metrics                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"

// NOTE:  These *are* in METR_TYPE order, so array[type] should work
const METTY metrics_types_defns[METR_TYPE_MAX] =
{
	{
		.name  = "Untyped",
		.nlen  = 0,
		.type  = METR_TYPE_UNTYPED,
		.dtype = DATA_TYPE_ADDER,
	},
	{
		.name  = "Counter",
		.nlen  = 7,
        .type  = METR_TYPE_COUNTER,
		.dtype = DATA_TYPE_ADDER,
	},
	{
		.name  = "Summary",
		.nlen  = 7,
		.type  = METR_TYPE_SUMMARY,
		.dtype = DATA_TYPE_ADDER,
	},
	{
		.name  = "Histogram",
		.nlen  = 9,
		.type  = METR_TYPE_HISTOGRAM,
		.dtype = DATA_TYPE_ADDER,
	},
	{
		.name  = "Gauge",
		.nlen  = 5,
		.type  = METR_TYPE_GAUGE,
		.dtype = DATA_TYPE_GAUGE,
	}
};


int metrics_cmp_attrs( const void *ap1, const void *ap2 )
{
	METMP *m1 = *((METMP **) ap1);
	METMP *m2 = *((METMP **) ap2);

	return ( m1->order < m2->order ) ? 1 : ( m1->order == m2->order ) ? 0 : -1;
}


void metrics_sort_attrs( MDATA *md )
{
	METMP **list, *m;
	int i = 0, j;

	if( !md->attrs )
		return;

	list = (METMP **) allocz( md->attct * sizeof( METMP * ) );
	for( m = md->attrs; m; m = m->next )
		list[i++] = m;

	qsort( list, md->attct, sizeof( METMP * ), metrics_cmp_attrs );

	// relink them and normalise the ordering
	j = md->attct - 1;
	for( i = 0; i < j; i++ )
	{
		list[i]->next  = list[i+1];
		list[i]->order = i;
	}
	list[j]->next  = NULL;
	list[j]->order = j;

	md->attrs = list[0];
	free( list );

	md->aps = (char **) allocz( md->attct * sizeof( char * ) );
	md->apl = (int16_t *) allocz( md->attct * sizeof( int16_t ) );
}


METRY *metrics_find_entry( MDATA *m, char *str, int len )
{
	uint64_t hval;
	METRY *e;

	hval = data_path_hash_wrap( str, len ) % m->hsz;

	for( e = m->entries[hval]; e; e = e->next )
		if( e->len == len && !memcmp( e->metric, str, len ) )
			return e;

	return NULL;
}


// it's in the words struct when this is called
// no locking because the mdata struct is only used
// by this thread
void metrics_add_entry( FETCH *f )
{
	const METTY *t, *tp;
	uint64_t hval;
	char *pm, *pt;
	int lm, lt, i;
	METRY *e;
	MDATA *m;

	m = f->metdata;

	pm = m->wds->wd[2];
	lm = m->wds->len[2];
	pt = m->wds->wd[3];
	lt = m->wds->len[3];

	hval = data_path_hash_wrap( pm, lm ) % m->hsz;

	for( e = m->entries[hval]; e; e = e->next )
		if( e->len == lm && !memcmp( pm, e->metric, lm ) )
			break;

	if( !e )
	{
		tp = NULL;

		// find the type
		for( t = metrics_types_defns, i = 0; i < METR_TYPE_MAX; i++, t++ )
			if( lt == t->nlen && !strncasecmp( pt, t->name, lt ) )
			{
				tp = t;
				break;
			}

		if( !tp )
		{
			warn( "Invalid metric type '%s' from fetch block %s.", pt, f->name );
			return;
		}

		e = mem_new_metry( pm, lm );
		e->mtype = (METTY *) tp;
		e->dtype = data_type_defns + tp->dtype;
		e->afp   = e->dtype->af;
		e->next  = m->entries[hval];
		m->entries[hval] = e;
		m->entct++;
	}
}



void metrics_parse_line( FETCH *f, char *line, int len )
{
	register char *q, *p = line;
	MDATA *m = f->metdata;
	int j, k, l, blen;
	char *r, *val;
	METRY *e;
	METMP *a;

	m = f->metdata;

	info( "Saw line [%03d]: %s", len, line );
	m->lines++;

	while( len > 0 && isspace( *p ) )
	{
		len--;
		p++;
	}

	// it's no use if it's shorter than this
	if( len < 8 )
		return;

	// we *should* get a type and a help line before we see metrics
	// we are ignoring help, as we don't send things to users
	if( *p == '#' )
	{
		if( strwords( m->wds, p, len, ' ' ) == 4
		 && m->wds->len[1] == 4 && !strncasecmp( m->wds->wd[1], "TYPE", 4 ) )
		{
			// yup, got a type, so add it
			metrics_add_entry( f );
		}

		return;
	}

	// the rest should be data lines
	// we are expecting:  ^metric({label="string",...})? value( timestamp)?$
	// so our approach is:
	// find the first space, check backwards for close brace
	// the next field is the value, but check for a space for a timestamp
	// we don't actually want the timestamp
	// if there was a close brace, go find the open brace, what's before it is the metric
	// check we know this metric, drop the line if we don't
	// if we do, we construct the new path based on attr ordering and submit it

	if( !( q = memchr( p, ' ', len ) ) )
	{
		// broken line
		m->broken++;
		return;
	}

	r = q;
	l = r - p;
	
	*q++ = '\0';
	len -= q - p;

	while( len > 0 && isspace( *q ) )
	{
		*q++ = '\0';
		len--;
	}

	// capture our val pointer
	val = q;

	// go looking for labels
	q = r - 1;
	if( *q != '}' )
	{
		// then we have a bare metric
	  	if( !( e = metrics_find_entry( m, p, l ) ) )
		{
			// unknown metric
			m->unknown++;
			return;
		}

		// if we have it, we can go for it
		(*(e->afp))( p, l, val );
		return;
	}
	r = q;

	// so we have labels - do we have a start?
	if( !( q = memchr( p, '{', l ) ) )
	{
		m->broken++;
		return;
	}

	// metric is in front of the labels
	if( !( e = metrics_find_entry( m, p, q - p ) ) )
	{
		// unknown metric again
		m->unknown++;
		return;
	}

	// do we care about attrs?
	if( m->attct == 0 )
	{
		(*(e->afp))( e->metric, e->len, val );
		return;
	}

	// so let's read our labels
	q++;
	*r = '\0';
	l = r - p;

	memset( m->aps, 0, m->attct * sizeof( char * ) );
	memset( m->apl, 0, m->attct * sizeof( int16_t ) );

	while( l > 0 )
	{
		// if we have a comma, just take the slice
		if( ( q = memchr( p, ',', l ) ) )
		{
			k = q - p;
			*q++ = '\0';
			r = q;
		}
		// otherwise take the whole thing
		else
			k = l;

		if( !( q = memchr( p, '=', k ) ) )
		{
			m->broken++;
			return;
		}

		// length of the attr
		j = q - p;

		// go looking for it in the order
		// capture a pointer to it and its length
		for( a = m->attrs; a; a = a->next )
			if( a->alen == j && !memcmp( a->attr, p, j ) )
			{
				// find the value
				q++;
				if( *q != '"' )
				{
					m->broken++;
					return;
				}

				q++;
				k -= q - p;
				if( q[k-1] != '"' )
				{
					m->broken++;
					return;
				}
				q[--k] = '\0';

				m->aps[a->order] = q;
				m->apl[a->order] = k;
				break;
			}

		// and move on
		p = r;
		l -= k;
	}

	// let's assemble our path then
	blen = e->len + 1;
	memcpy( m->buf, e->metric, e->len );
	m->buf[e->len] = '.';

	for( j = 0; j < m->attct; j++ )
		if( m->apl[j] == 0 )
		{
			memcpy( m->buf + blen, m->aps[j], m->apl[j] );
			blen += m->apl[j];
			m->buf[blen - 1] = '.';
		}
		else
		{
			memcpy( m->buf + blen, "unknown.", 8 );
			blen += 8;
		}

	// chop off the last .
	m->buf[--blen] = '\0';

	info( "Submit: %s %s", m->buf, val );

	// and we are ready to go - process that
	(*(e->afp))( m->buf, blen, val );
}




void metrics_parse_buf( FETCH *f, IOBUF *b )
{
	register char *s = b->buf;
	register char *q;
	int len, l;
	char *r;

	// we need attributes and other things from the fetch
	if( !f )
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
				b->len = len;
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

		// clean leading and trailing \r's
		while( *s == '\r' )
		{
			s++;
			l--;
		}
		if( l > 0 && *r == '\r' )
		{
			*r = '\0';
			l--;
		}

		// still got anything?  process it
		if( l > 0 )
			metrics_parse_line( f, s, l );

		// and move on
		s = q;
	}

	// and update the buffer length
	b->len = len;
}




void metrics_init_data( MDATA *m )
{
	// make an entry hash
	m->entries = (METRY **) allocz( m->hsz * sizeof( METRY * ) );

	// make a words struct
	m->wds = (WORDS *) allocz( sizeof( WORDS ) );

	// a metric buffer
	m->buf = (char *) allocz( METR_BUF_SZ );

	// and sort our attrs
	metrics_sort_attrs( m );
}



int metrics_add_attr( MDATA *m, char *str, int len )
{
	METMP tmp, *a;
	char *cl;

	memset( &tmp, 0, sizeof( METMP ) );

	// were we given an order?
	if( ( cl = memchr( str, ':', len ) ) )
	{
		*cl++ = '\0';
		len -= cl - str;
		tmp.order = atoi( str );
		str = cl;
	}
	// if not use order of processing
	else
		tmp.order = m->attct;

	if( !len )
	{
		err( "Empty attribute name." );
		return -1;
	}

	tmp.attr = str_dup( str, len );
	tmp.alen = len;

	a = (METMP *) allocz( sizeof( METMP ) );
	memcpy( a, &tmp, sizeof( METMP ) );

	a->next  = m->attrs;
	m->attrs = a;
	m->attct++;

	return 0;
}


