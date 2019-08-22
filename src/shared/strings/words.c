/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* strings/words.c - string util to break a string into words              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"



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


