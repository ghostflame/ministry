/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* data.c - buffer processing, data ingestion functions                    *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "archivist.h"




LEAF *data_process_line( char *str, int len )
{
	int i, last, clen;
	TEL *t, *prt;
	char *copy;
	WORDS w;
	LEAF *l;

	// do we already have that one?
	if( ( l = rkv_hash_find( str, len ) ) )
		return l;

	prt = ctl->rkv->root;
	copy = str_copy( str, len );
	clen = 0;
	l = NULL;

	strwords( &w, str, len, '.' );
	last = w.wc - 1;

	for( i = 0; i < w.wc; i++ )
	{
		for( t = prt->child; t; t = t->next )
			if( t->len == w.len[i] && !memcmp( t->name, w.wd[i], t->len ) )
				break;

		if( !t )
		{
			if( i == last )
				clen = len;

			// leaf/node mismatch?
			if( rkv_tree_insert( w.wd[i], w.len[i], copy, clen, prt, &t ) < 0 )
				break;
		}
		else
		{
			// leaf/node mismatch?
			if( t->leaf && i < last )
				break;
		}

		l = t->leaf;
		prt = t;
	}

	free( copy );
	return l;
}



void data_handle_line( HOST *h, char *str, int len )
{
	char *sp, *path;
	int64_t ts, now;
	double val;
	LEAF *l;
	PTL *p;
	int pl;

	//info( "Saw line: [%d] %s", len, str );

	if( !( sp = memchr( str, ' ', len ) ) )
	{
		info( "No first space in the line." );
		++(h->invalid);
		return;
	}

	*sp  = '\0';
	path = str;
	pl   = sp - str;
	++sp;

	len -= pl + 1;
	str += pl + 1;

	if( !( sp = memchr( str, ' ', len ) ) )
	{
		++(h->invalid);
		return;
	}

	while( *sp == ' ' && len > 0 )
	{
		*sp++ = '\0';
		--len;
	}

	if( !( l = data_process_line( path, pl ) ) )
	{
		++(h->invalid);
		return;
	}

	++(h->lines);

	val = strtod( str, NULL );
	ts  = strtoll( sp, NULL, 10 );
	now = _proc->curr_usec;

	rkv_tree_lock( l->tel );

	if( !( p = l->points ) || p->count >= p->sz )
	{
		p = mem_new_ptlst( );
		p->next = l->points;
		l->points = p;
	}

	p->points[p->count].val = val;
	p->points[p->count].ts  = ts;
	++(p->count);

	if( !l->oldest )
		l->oldest = now;

	rkv_tree_unlock( l->tel );

	l->last_updated = now;
	++(h->handled);
}


void data_handle_record( HOST *h, char *str, int len )
{
	return;
}



int data_parse_line( HOST *h, IOBUF *b )
{
	register char *s = b->bf->buf;
	register char *q;
	register int l;
	int len;
	char *r;

	len = b->bf->len;

	while( len > 0 )
	{
		// look for newlines
		if( !( q = memchr( s, LINE_SEPARATOR, len ) ) )
		{
			// partial last line
			// and we're done, with len > 0
			break;
		}

		l = q - s;
		r = q - 1;

		// delete the newline
		*q++ = '\0';

		// decrement len
		len -= q - s;

		// clean up the line of '\r'
		if( *s == '\r' )
		{
			++s;
			--l;
		}

		// catch trailing \r's
		if( l > 0 && *r == '\r' )
		{
			*r = '\0';
			--l;
		}

		// if we have anything, process it
		if( l > 0 )
			(*(h->parser))( h, s, l );

		// and move on
		s = q;
	}

	strbuf_keep( b->bf, len );
	return len;
}

int data_parse_bin( HOST *h, IOBUF *b )
{
	return 0;
}

