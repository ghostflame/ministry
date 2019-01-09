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
	METAT *m1 = *((METAT **) ap1);
	METAT *m2 = *((METAT **) ap2);

	return ( m1->order < m2->order ) ? -1 : ( m1->order == m2->order ) ? 0 : 1;
}

int metrics_cmp_maps( const void *ap1, const void *ap2 )
{
	METMP *m1 = *((METMP **) ap1);
	METMP *m2 = *((METMP **) ap2);

	return ( m1->id < m2->id ) ? -1 : ( m1->id == m2->id ) ? 0 : 1;
}


void metrics_profile_sort_maps( METPR *p )
{
	METMP **list, *m;
	int i = 0, j;

	if( !p->maps )
		return;

	list = (METMP **) allocz( p->mapct * sizeof( METMP * ) );
	for( m = p->maps; m; m = m->next )
		list[i++] = m;

	qsort( list, p->mapct, sizeof( METPR * ), metrics_cmp_maps );

	// relink them
	j = p->mapct - 1;
	for( i = 0; i < j; i++ )
		list[i]->next = list[i+1];

	list[j]->next = NULL;
	p->maps = list[0];
	free( list );
}


void metrics_sort_attrs( METAL *a )
{
	METAT **list, *m;
	int i = 0, j;

	if( !a->ats )
		return;

	list = (METAT **) allocz( a->atct * sizeof( METAT * ) );
	for( m = a->ats; m; m = m->next )
		list[i++] = m;

	qsort( list, a->atct, sizeof( METAT * ), metrics_cmp_attrs );

	// relink them and normalise the ordering
	j = a->atct - 1;
	for( i = 0; i < j; i++ )
	{
		list[i]->next  = list[i+1];
		list[i]->order = i;
	}
	list[j]->next  = NULL;
	list[j]->order = j;

	a->ats = list[0];
	free( list );

	/* TODO
	 * Needs moving to the metric-to-list resolver
	// leave room for quantile or le and a bucket
	md->aps = (char **) allocz( ( 2 + a->atct ) * sizeof( char * ) );
	md->apl = (int16_t *) allocz( ( 2 + a->atct ) * sizeof( int16_t ) );
	*/
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
void metrics_add_entry( FETCH *f, METRY *parent )
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
		e->mtype  = (METTY *) tp;
		e->dtype  = data_type_defns + tp->dtype;
		e->afp    = e->dtype->af;
		e->next   = m->entries[hval];
		e->parent = parent;
		m->entries[hval] = e;
		m->entct++;

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
	}
}


static inline int metrics_check_attr( MDATA *m, int which, char *attr, int order )
{
	register char *p, *q;
	int i, l, k;
	METAT *a;

	p = m->wds->wd[which];
	l = m->wds->len[which];

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
			m->aps[order] = q;
			m->apl[order] = l;

			// known case - quantile.  Flatten the dots
			for( i = 0; i < l; i++, q++ )
				if( *q == '.' || *q == '+' )
					*q = '_';
		}

		return 0;
	}

	// go looking for it in the order
	// capture a pointer to it and its length
	for( a = m->attrs->ats; a; a = a->next )
		if( a->len == k && !memcmp( a->name, p, k ) )
		{
			//info( "Matched attribute '%s', order %d.", a->attr, a->order );
			m->aps[a->order] = q;
			m->apl[a->order] = l;
			break;
		}

	return 0;
}



void metrics_parse_line( FETCH *f, char *line, int len )
{
	register char *q, *p = line;
	int j, l, blen, attct;
	MDATA *m = f->metdata;
	char *r, *val;
	METRY *e;

	m = f->metdata;

	//info( "Saw line [%03d]: %s", len, line );
	m->lines++;

	while( len > 0 && isspace( *p ) )
	{
		len--;
		p++;
	}

	// it's no use if it's shorter than this
	if( len < 8 )
	{
		//info( "Dropped line - too short." );
		return;
	}

	// we *should* get a type and a help line before we see metrics
	// we are ignoring help, as we don't send things to users
	if( *p == '#' )
	{
		if( strwords( m->wds, p, len, ' ' ) == 4
		 && m->wds->len[1] == 4 && !strncasecmp( m->wds->wd[1], "TYPE", 4 ) )
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

	if( !( q = memchr( p, ' ', len ) ) )
	{
		// broken line
		m->broken++;
		debug( "Line broken - no space." );
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
			debug( "Line dropped - metric '%s' unknown.", p );
			return;
		}

		// if we have it, we can go for it
		//info( "Submitting with no labels: (%d) %s %s", l, p, val );
		(*(e->afp))( p, l, val );
		return;
	}
	r = q;

	// so we have labels - do we have a start?
	if( !( q = memchr( p, '{', l ) ) )
	{
		m->broken++;
		debug( "Line broken - } but no {" );
		return;
	}

	*q = '\0';

	// metric is in front of the labels
	if( !( e = metrics_find_entry( m, p, q - p ) ) )
	{
		// unknown metric again
		m->unknown++;
		debug( "Line broken - labelled metric '%s' unknown.", p );
		return;
	}

	// do we care about attrs?
	if( m->attct == 0 )
	{
		//info( "Submitting without attrs: (%d) %s %s", e->len, e->metric, val );
		(*(e->afp))( e->metric, e->len, val );
		return;
	}

	// so let's read our labels
	q++;
	*r = '\0';
	l = r - q;

	attct = m->attct;
	if( e->mtype->type == METR_TYPE_SUMMARY || e->mtype->type == METR_TYPE_HISTOGRAM )
		attct += 2;

	memset( m->aps, 0, attct * sizeof( char * ) );
	memset( m->apl, 0, attct * sizeof( int16_t ) );

	strwords( m->wds, q, l, ',' );

	for( j = 0; j < m->wds->wc; j++ )
		if( metrics_check_attr( m, j, NULL, 0 ) != 0 )
			return;

	// hack - last metric is quantile for summaries
	// push it onto the end of the list
	if( e->mtype->type == METR_TYPE_SUMMARY )
	{
		m->aps[m->attct] = "stat";
		m->apl[m->attct] = 4;
		metrics_check_attr( m, m->wds->wc - 1, "quantile", m->attct + 1 );
	}
	else if( e->mtype->type == METR_TYPE_HISTOGRAM )
	{
		m->aps[m->attct] = "le";
		m->apl[m->attct] = 2;
		metrics_check_attr( m, m->wds->wc - 1, "le", m->attct + 1 );
	}

	// let's assemble our path then
	blen = e->len + 1;
	memcpy( m->buf, e->metric, e->len );
	m->buf[e->len] = '.';

	for( j = 0; j < attct; j++ )
		if( m->apl[j] > 0 )
		{
			memcpy( m->buf + blen, m->aps[j], m->apl[j] );
			blen += m->apl[j];
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


void metrics_fetch_cb( void *arg, IOBUF *b )
{
	metrics_parse_buf( (FETCH *) arg, b );
}



void metrics_init_data( MDATA *m )
{
	// make an entry hash
	m->entries = (METRY **) allocz( m->hsz * sizeof( METRY * ) );

	// make a words struct
	m->wds = (WORDS *) allocz( sizeof( WORDS ) );

	// a metric buffer
	m->buf = (char *) allocz( METR_BUF_SZ );
}


// we are expecting path:list-name
int metrics_add_path( METPR *p, char *str, int len )
{
	char *cl;
	SSTE *e;
	int l;

	if( !( cl = memchr( str, ':', len ) ) )
		return -1;

	l = len - ( cl - str );
	*cl++ = '\0';

	if( !p->paths )
		p->paths = string_store_create( 0, "small", 1 );

	if( !( e = string_store_add( p->paths, str, len ) ) )
		return -1;

	// copy the name of the list for now
	// we will have to resolve them later
	e->ptr = str_copy( cl, l );

	return 0;
}

METAL *metrics_get_alist( char *name )
{
	METAL *m;
	int l;

	l = strlen( name );

	for( m = ctl->metric->alists; m; m = m->next )
		if( l == m->len && !strncasecmp( m->name, name, l ) )
			return m;

	err( "Could not resolve matrics attribute list for name '%s'", name );

	return NULL;
}

int metrics_add_attr( METAL *m, char *str, int len )
{
	METAT tmp, *a;
	char *cl;

	memset( &tmp, 0, sizeof( METAT ) );

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
		tmp.order = m->atct;

	if( !len )
	{
		err( "Empty attribute name." );
		return -1;
	}

	tmp.name = str_dup( str, len );
	tmp.len  = len;

	a = (METAT *) allocz( sizeof( METAT ) );
	memcpy( a, &tmp, sizeof( METAT ) );

	a->next = m->ats;
	m->ats  = a;
	m->atct++;

	return 0;
}

/*
 * Go through each metrics attr lists and sort them
 */
int metrics_init( void )
{
	METAL *a;
	METPR *p;
	METMP *m;

	// sort the attributes in our attr lists
	for( a = ctl->metric->alists; a; a = a->next )
		metrics_sort_attrs( a );

	// resolve the attr list names in our profiles
	for( p = ctl->metric->profiles; p; p = p->next )
	{
		if( p->default_att
		 && !( p->default_attrs = metrics_get_alist( p->default_att ) ) )
			return -1;

		for( m = p->maps; m; m = m->next )
			if( !( m->list = metrics_get_alist( m->lname ) ) )
				return -2;
	}

	return 0;
}



MET_CTL *metrics_config_defaults( void )
{
	MET_CTL *m = (MET_CTL *) allocz( sizeof( MET_CTL ) );

	// what here?

	return m;
}


static METAL __metrics_metal_tmp;
static int __metrics_metal_state = 0;

static METPR __metrics_prof_tmp;
static int __metrics_prof_state = 0;

#define _m_inter	{ err( "Metrics attr list and profile config interleaved." ); return -1; }
#define _m_mt_chk	if( __metrics_metal_state ) _m_inter
#define _m_ps_chk	if( __metrics_prof_state )  _m_inter

int metrics_config_line( AVP *av )
{
	METAL *na, *a = &__metrics_metal_tmp;
	METPR *np, *p = &__metrics_prof_tmp;
	METAT *ap;
	METMP *mp;
	int64_t v;

	if( !__metrics_metal_state )
	{
		if( a->name )
			free( a->name );
		while( ( ap = a->ats ) )
		{
			if( ap->attr )
				free( ap->attr );
			a->ats = ap->next;
			free( ap );
		}
		memset( a, 0, sizeof( METAT ) );
	}
	if( !__metrics_prof_state )
	{
		if( p->name )
			free( p->name );
		memset( p, 0, sizeof( METPR ) );
		p->_id_ctr = 1024;
	}

	if( attIs( "list" ) )
	{
		_m_ps_chk;

		if( a->name )
		{
			warn( "Attribute map '%s' already had a name.", a->name );
			free( a->name );
		}

		a->name = str_copy( av->val, av->vlen );
		a->nlen = av->vlen;
		__metrics_metal_state = 1;
	}
	else if( attIs( "attribute" ) || attIs( "attr" ) )
	{
		_m_ps_chk;
		__metrics_metal_state = 1;
		return metrics_add_attr( a, av->val, av->vlen );
	}

	// profile values
	else if( attIs( "profile" ) )
	{
		_m_mt_chk;

		if( p->name )
		{
			warn( "Profile '%s' already had a name.", p->name );
			free( p->name );
		}

		p->name = str_copy( av->val, av->vlen );
		p->nlen = av->vlen;
		__metrics_prof_state = 1;
	}
	else if( attIs( "default" ) )
	{
		_m_mt_chk;

		if( p->default_att )
			free( p->default_att );

		p->default_att = str_copy( av->val, av->vlen );
	}
	else if( attIs( "path" ) )
	{
		_m_mt_chk;
		__metrics_prof_state = 1;
		return metrics_add_path( p, av->val, av->vlen );
	}
	else if( !strncasecmp( av->att, "map.", 4 ) )
	{
		_m_mt_chk;

		av->att  += 4;
		av->alen -= 4;

		if( attIs( "begin" ) )
		{
			mp = (METMP *) allocz( sizeof( METMP ) );
			mp->next = p->maps;
			p->maps  = mp;

			mp->id = p->_id_ctr++;
			mp->enable = 1;
			p->mapct++;
		}
		else if( attIs( "enable" ) )
		{
			p->maps->enable = config_bool( av );
		}
		else if( attIs( "list" ) )
		{
			if( p->maps->lname )
				free( p->maps->lname );

			p->maps->lname = str_copy( av->val, av->vlen );
		}
		else if( attIs( "id" ) )
		{
			av_int( v );
			p->maps->id = (int) v;
		}
		else if( attIs( "last" ) )
		{
			p->maps->last = config_bool( av );
		}
		else if( !strncasecmp( av->att, "regex.", 6 ) )
		{
			av->att  += 6;
			av->alen -= 6;

			if( !p->maps->rlist )
				p->maps->rlist = regex_list_create( 1 );

			if( attIs( "fallbackMatch" ) )
			{
				regex_list_set_fallback( config_bool( av ), p->maps->rlist );
			}
			else if( attIs( "match" ) )
			{
				return regex_list_add( av->val, 0, p->maps->rlist );
			}
			else if( attIs( "fail" ) )
			{
				return regex_list_add( av->val, 1, p->maps->rlist );
			}
		}

		__metrics_prof_state = 1;
	}

	else if( attIs( "done" ) )
	{
		// work out which one it is
		if( __metrics_metal_state )
		{
			if( !a->name )
			{
				err( "Cannot proceed with unnamed metrics attribute list." );
				return -1;
			}

			na = (METAL *) allocz( sizeof( METAL ) );
			memcpy( na, a, sizeof( METAL ) );

			na->next = ctl->metric->alists;
			ctl->metal->alists = na;
			ctl->metal->alist_ct++;

			__metrics_metal_state = 0;
		}
		else if( __metrics_prof_state )
		{
			if( !p->name )
			{
				err( "Cannot proceed with unnamed metrics profile." );
				return -1;
			}

			if( !p->maps || !p->paths || !p->paths->entries )
			{
				err( "Metrics profile '%s' will not match anything.  Aborting." );
				return -1;
			}

			// sort the maps into ID order
			metrics_profile_order_maps( p );

			for( mp = p->maps; mp; mp = mp->next )
				if( !mp->lname )
				{
					if( p->default_att )
					{
						info( "Metrics profile '%s', map id %d falling back to default attr list '%s'.",
							p->name, mp->id, p->default_att );
						mp->lname = p->default_att;
					}
					else
					{
						err( "Metrics profile '%s', map id %d, has no attribute list target name.", p->name, mp->id );
						return -1;
					}
				}

			np = (METPR *) allocz( sizeof( METPR ) );
			memcpy( np, p, sizeof( METPR ) );

			np->next = ctl->metric->profiles;
			ctl->metric->profiles = np;
			ctl->metric->prof_ct++;

			__metric_prof_state = 0;
		}
		else
		{
			err( "Done called in metrics config for neither an attribute list nor a profile." );
			return -1;
		}
	}
	else
		return -1;

	return 0;
}


#undef _m_inter
#undef _m_mt_chk
#undef _m_ps_chk

