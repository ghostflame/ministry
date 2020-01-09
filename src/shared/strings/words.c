/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* strings/words.c - string util to break a string into words              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



int strqwords( WORDS *w, char *src, int len, char sep )
{
	register char *p = src;
	register char *q = NULL;
	register int   l;
	int i = 0, qc = 0, qchar;

	if( !w || !p || !sep )
		return -1;

	if( !len && !( len = strlen( p ) ) )
		return 0;

	l = len;

	memset( w, 0, sizeof( WORDS ) );

	w->sep = sep;

	w->in_len = l;
	qchar     = '"';

	// step over leading separators
	while( *p == sep )
	{
		++p;
		l--;
	}

	// anything left?
	if( !*p )
		return 0;

	while( l > 0 )
	{
		// decision: "" wraps ''
		if( *p == '"' )
		{
			qc = 1;
			qchar = '"';
		}

		if( !qc && *p == '\'' )
		{
			qc = 1;
			qchar = '\'';
		}

		if( qc )
		{
			++p;
			l--;

			w->wd[i] = p;

			if( ( q = memchr( p, qchar, l ) ) )
			{
				// capture inside the quotes
				w->len[i++] = q - p;
				*q++ = '\0';
				l -= q - p;
				p = q;
				qc = 0;
			}
			else
			{
				// invalid string - uneven quotes
				// assume to-end-of-line for the
				// quoted string
				w->len[i++] = l;
				break;
			}

			// step over any separators following the quotes
			while( *p == sep )
			{
				++p;
				l--;
			}
		}
		else
		{
			w->wd[i] = p;

			if( ( q = memchr( p, sep, l ) ) )
			{
				w->len[i++] = q - p;
				*q++ = '\0';
				l -= q - p;
				p = q;

				if( !l )
					w->end_on_sep = 1;
			}
			else
			{
				w->len[i++] = l;
				break;
			}
		}

		w->end = p;
		w->end_len = l;

		if( i == STRWORDS_MAX )
			break;
	}

	return ( w->wc = i );
}



int strwords_multi( WORDS *w, char *src, int len, char sep, int8_t multi )
{
	register char *p = src;
	register char *q = NULL;
	register int   l;
	int i = 0;

	if( !w || !p || !sep )
		return -1;

	if( !len && !( len = strlen( p ) ) )
		return 0;

	l = len;

	memset( w, 0, sizeof( WORDS ) );

	w->sep = sep;
	w->in_len = l;

	// step over leading separators
	while( *p == sep )
	{
		++p;
		l--;
	}

	// anything left?
	if( !*p )
		return 0;

	// and break it up
	while( l )
	{
		w->wd[i] = p;

		if( ( q = memchr( p, sep, l ) ) )
		{
			w->len[i++] = q - p;
			if( multi )
				while( *q == sep )
					*q++ = '\0';
			else
				*q++ = '\0';

			l -= q - p;
			p = q;

			if( !l )
				w->end_on_sep = 1;
		}
		else
		{
			w->len[i++] = l;
			break;
		}

		w->end = p;
		w->end_len = l;

		if( i == STRWORDS_MAX )
			break;
	}

	// done
	return ( w->wc = i );
}


// join a words structure on a filler.  No filler is legal.
// either allocate a string (or free an alloc, is *dst is set but *sz is 0)
int strwords_join( WORDS *w, char *filler, char **dst, int *sz )
{
	int i, len, fl;
	char *p;

	if( !dst || !sz || !w )
		return -1;

	if( !w->wc )
		return -2;

	if( !filler )
		filler = "";

	// figure out our length
	fl = strlen( filler );
	len = ( w->wc - 1 ) * fl;

	for( i = 0; i < w->wc; ++i )
		len += w->len[i];

	// do we have a string?
	if( *dst )
	{
		if( !*sz )
		{
			free( *dst );
			*dst = NULL;
		}
		else if( len >= *sz )
		{
			// too big
			return -3;
		}
	}

	// make some space
	p = *dst = (char *) allocz( len + 1 );

	// start with the first word
	memcpy( p, w->wd[0], w->len[0] );
	p += w->len[0];

	// then add filler/word/filler/word until done
	for( i = 1; i < w->wc; ++i )
	{
		if( fl )
		{
			memcpy( p, filler, fl );
			p += fl;
		}

		memcpy( p, w->wd[i], w->len[i] );
		p += w->len[i];
	}

	*sz = len;
	return 0;
}

int strwords_json_keys( WORDS *w, JSON *jo )
{
	struct lh_table *t;
	struct lh_entry *e;
	int i = 0;

	if( !w || !jo )
		return -1;

	memset( w, 0, sizeof( WORDS ) );

	if( !( t = json_object_get_object( jo ) ) )
		return -2;

	// too many keys?
	if( t->count > STRWORDS_MAX )
	{
		warn( "Too many keys (%d) in json object to put into WORDS.", t->count );
		return -3;
	}

	lh_foreach( t, e )
	{
		w->wd[i] = (char *) e->k;
		w->len[i] = strlen( w->wd[i] );
		++i;
	}

	w->wc = i;
	return i;
}

