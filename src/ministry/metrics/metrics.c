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
*                                                                         *
* metrics/metrics.c - functions for prometheus-style metrics              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


// it's in the words struct when this is called
// no locking because the mdata struct is only used
// by this thread
void metrics_add_entry( FETCH *f, METRY *parent )
{
	const METTY *t;
	char *pm, *pt;
	int lm, lt, i;
	METMP *map;
	METPR *pr;
	METRY *e;
	MDATA *m;
	SSTE *se;

	m = f->metdata;

	pm = m->wds->wd[2];
	lm = m->wds->len[2];

	// do we already have it?
	if( ( se = string_store_look( m->entries, pm, lm, 0 ) ) )
		return;

	if( parent )
	{
		t = parent->mtype;
	}
	else
	{
		pt = m->wds->wd[3];
		lt = m->wds->len[3];

		// find the type
		for( t = metrics_types_defns, i = 0; i < METR_TYPE_MAX; ++i, ++t )
			if( lt == t->nlen && !strncasecmp( pt, t->name, lt ) )
				break;

		if( i == METR_TYPE_MAX )
		{
			warn( "Invalid metric type '%s' from fetch block %s.", pt, f->name );
			return;
		}
	}

	e = mem_new_metry( pm, lm );
	e->mtype  = t;
	e->dtype  = data_type_defns + t->dtype;
	e->afp    = e->dtype->af;
	e->parent = parent;

	if( !( se = string_store_add( m->entries, pm, lm ) ) )
	{
		mem_free_metry( &e );
		return;
	}

	// and save the entry
	se->ptr = e;

	debug( "Added entry '%s', type %s, for fetch block %s.", e->metric, e->mtype->name, f->name );

	// for summaries we need to create the _sum and _count entries too, parented off the main one
	// and histograms need a _bucket too, and don't even use the main type.  WTF prometheus.
	if( ( e->mtype->type == METR_TYPE_SUMMARY || e->mtype->type == METR_TYPE_HISTOGRAM )
	 && !e->parent )
	{
		char pathbuf[2048];

		m->wds->wd[2]  = pathbuf;
		m->wds->wd[3]  = "counter";
		m->wds->len[3] = 7;

		m->wds->len[2] = snprintf( pathbuf, 2048, "%s_sum", pm );
		metrics_add_entry( f, e );

		m->wds->len[2] = snprintf( pathbuf, 2048, "%s_count", pm );
		metrics_add_entry( f, e );

		if( e->mtype->type == METR_TYPE_HISTOGRAM )
		{
			m->wds->wd[3]  = "histogram";
			m->wds->len[3] = 9;

			m->wds->len[2] = snprintf( pathbuf, 2048, "%s_bucket", pm );
			metrics_add_entry( f, e );
		}
	}

	// resolve our profile
	pr = m->profile;

	// is it a direct path entry?
	if( ( se = string_store_look( pr->paths, e->metric, e->len, 0 ) ) )
	{
		e->attrs = (METAL *) se->ptr;
		return;
	}

	// ok, let's try the maps
	for( map = pr->maps; map; map = map->next )
	{
		if( regex_list_test( e->metric, map->rlist ) == REGEX_MATCH )
		{
			e->attrs = map->list;
			if( map->last )
				break;
		}
	}
	if( !e->attrs )
		e->attrs = pr->default_attrs;

	// and create our scratch space - leave some room
	e->aps = (char **)   allocz( ( 3 + e->attrs->atct ) * sizeof( char * ) );
	e->apl = (int16_t *) allocz( ( 3 + e->attrs->atct ) * sizeof( int16_t ) );
}


static inline int metrics_check_attr( METRY *e, char *p, int l, char *attr, int order )
{
	register char *q;
	int i, k;
	METAT *a;

	if( p[--l] != '"' )
	{
		debug( "Line broken - label with no second \": %s", p );
		return -1;
	}

	if( !( q = memchr( p, '=', l ) ) )
	{
		debug( "Line broken - label with no =." );
		return -2;
	}

	k = q++ - p;

	if( *q++ != '"' )
	{
		debug( "Line broken - label with no first \": %s", p );
		return -3;
	}

	l -= q - p;

	// are we doing a single comparison?
	// check it and always return
	if( attr )
	{
		i = strlen( attr );
		if( k == i && !memcmp( p, attr, i ) )
		{
			e->aps[order] = q;
			e->apl[order] = l;

			// known case - quantile.  Flatten the dots
			for( i = 0; i < l; ++i, ++q )
				if( *q == '.' || *q == '+' )
					*q = '_';
		}

		return 0;
	}

	// go looking for it in the order
	// capture a pointer to it and its length
	for( a = e->attrs->ats; a; a = a->next )
		if( a->len == k && !memcmp( a->name, p, k ) )
		{
			//info( "Matched attribute '%s', order %d.", a->attr, a->order );
			e->aps[a->order] = q;
			e->apl[a->order] = l;
			break;
		}

	return 0;
}



void metrics_parse_line( FETCH *f, char *line, int len )
{
	int j, attlen, metlen, blen, attct;
	char *q, *r, *val, *p = line;
	MDATA *m = f->metdata;
	METRY *e;

	//info( "Saw line [%03d]: %s", len, line );
	++(m->lines);

	// we *should* get a type and a help line before we see metrics
	// we are ignoring help, as we don't send anything to users
	if( *p == '#' )
	{
		if( strwords( m->wds, p, len, ' ' ) == 4
		 && m->wds->len[1] == 4
		 && !strncasecmp( m->wds->wd[1], "TYPE", 4 ) )
		{
			// yup, got a type, so add it
			metrics_add_entry( f, NULL );
		}

		//info( "Dropped line - comment." );
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


	// try for an open brace.  If we get one, try for a close brace and a space
	// fall back to just trying for a space
	// if you put '} ' inside an attribute value, good luck to you, but we don't

	if( ( q = memchr( p, '{', len ) ) )
	{
		metlen = q - p;
		len -= metlen;

		if( !( r = memchr( q, '}', len ) ) || *(r+1) != ' ' )
		{
			++(m->broken);
			debug( "Line broken - { but no } ." );
			return;
		}

		// len = 50
		// q is at + 20
		// len becomes 30
		// metlen becomes 20
		// r is at + 40
		// attlen is 19
		// len should become 9
		*q++   = '\0';
		attlen = ( r - q );
		*r++   = '\0';
		r++;
		len   -= attlen + 2;

		if( memchr( r, '}', len ) )
		{
			++(m->broken);
			debug( "Line broken - found two }'s." );
			return;
		}

		trim( &q, &attlen );
	}
	else if( ( r = memchr( p, ' ', len ) ) )
	{
		attlen = 0;
		metlen = r - p;
		*r++   = '\0';
		len   -= metlen + 1;
	}
	else
	{
		// broken line
		++(m->broken);
		debug( "Line broken - no space." );
		return;
	}

	// p points to the metric and metlen is right
	// q points to the attrs and attlen is right - q might be garbage
	// r points to the value
	// len is right

	trim( &p, &metlen );
	val = r;

	// find the metric
	if( !( e = metrics_find_entry( m, p, metlen ) ) )
	{
		++(m->unknown);
		debug( "Line dropped - metric '%s' unknown.", p );
		return;
	}

	// are there attrs, and do we care?
	if( !attlen || e->attrs->atct == 0 )
	{
		// then we have a metric with no labels
		(*(e->afp))( e->metric, e->len, val );
		return;
	}


	// so let's read our labels
	attct = e->attrs->atct;
	if( e->mtype->type == METR_TYPE_SUMMARY || e->mtype->type == METR_TYPE_HISTOGRAM )
		attct += 2;

	memset( e->aps, 0, attct * sizeof( char * ) );
	memset( e->apl, 0, attct * sizeof( int16_t ) );

	// don't put , in your label values either
	strwords( m->wds, q, attlen, ',' );

	for( j = 0; j < m->wds->wc; ++j )
		if( metrics_check_attr( e, m->wds->wd[j], m->wds->len[j], NULL, 0 ) != 0 )
			return;

	// hack - last metric is quantile for summaries
	// push it onto the end of the list
	if( e->mtype->type == METR_TYPE_SUMMARY )
	{
		e->aps[e->attrs->atct] = "stat";
		e->apl[e->attrs->atct] = 4;
		j = m->wds->wc - 1;
		metrics_check_attr( e, m->wds->wd[j], m->wds->len[j], "quantile", e->attrs->atct + 1 );
	}
	else if( e->mtype->type == METR_TYPE_HISTOGRAM )
	{
		e->aps[e->attrs->atct] = "le";
		e->apl[e->attrs->atct] = 2;
		j = m->wds->wc - 1;
		metrics_check_attr( e, m->wds->wd[j], m->wds->len[j], "le", e->attrs->atct + 1 );
	}

	// let's assemble our path then
	blen = e->len + 1;
	memcpy( m->buf, e->metric, e->len );
	m->buf[e->len] = '.';

	for( j = 0; j < attct; ++j )
		if( e->apl[j] > 0 )
		{
			memcpy( m->buf + blen, e->aps[j], e->apl[j] );
			blen += e->apl[j];
			m->buf[blen++] = '.';
		}
		else
		{
			memcpy( m->buf + blen, "unknown.", 8 );
			blen += 8;
		}

	// chop off the last .
	m->buf[--blen] = '\0';

	// and we are ready to go - process that
	//info( "Submitting with attrs: (%d) %s %s", blen, m->buf, val );
	(*(e->afp))( m->buf, blen, val );
}



void metrics_fetch_cb( void *arg, IOBUF *b )
{
	FETCH *f = (FETCH *) arg;
	char *q, *s = b->bf->buf;
	int len, l;

	// we need attributes and other things from the fetch
	if( !f )
		return;

	len = b->bf->len;

	while( len > 0 )
	{
		// look for newlines
		if( !( q = memchr( s, LINE_SEP_CHAR, len ) ) )
			// and we're done, with len > 0
			break;

		l = q - s;

		// stomp on that newline
		*q++ = '\0';

		// and decrement the remaining length
		len -= q - s;

		// tidy up that string
		trim( &s, &l );

		// still got anything?  process it
		if( l > 4 )
			metrics_parse_line( f, s, l );

		// and move on
		s = q;
	}

	// and update the buffer length
	strbuf_keep( b->bf, len );
}



