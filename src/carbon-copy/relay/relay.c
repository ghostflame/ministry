/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* relay/relay.c - handles connections and processes lines                 *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"



__attribute__((hot)) static inline void relay_write( HBUFS *hb, int idx, RLINE *l )
{
	IOBUF *b = hb->bufs[idx];

	// see if we need to write
	if( !buf_hasspace( b->bf, l->len ) )
	{
		io_buf_post( hb->rule->targets[idx], b );
		hb->bufs[idx] = b = mem_new_iobuf( IO_BUF_SZ );
	}

	// reconstruct the line
	l->line[l->plen] = l->sep;

	// add it to the buffer
	buf_appends( b->bf, l->line, l->len );
	// and the newline
	buf_addchar( b->bf, '\n' );
}



__attribute__((hot)) int relay_regex( HBUFS *h, RLINE *l )
{
	RELAY *r = h->rule;
	uint8_t mat;
	int j;

	// try each regex
	for( j = 0; j < r->mcount; ++j )
	{
		mat  = regexec( r->matches + j, l->line, 0, NULL, 0 ) ? 0 : 1;
		mat ^= r->invert[j];

		if( mat )
			break;
	}

	// no match?
	if( j == r->mcount )
		return 0;

	// are we dropping it?
	if( r->drop )
		return 1;

	// regex match writes to every target
	relay_write( h, 0, l );

	// and we matched
	return 1;
}



__attribute__((hot)) int relay_hash( HBUFS *h, RLINE *l )
{
	uint32_t j = 0;

	// if we are dropping, then just drop it
	if( h->rule->drop )
		return 1;

	if( h->rule->tcount > 1 )
		j = (*(h->rule->hfp))( l->line, l->plen ) % h->rule->tcount;

	// hash writes to just one target
	relay_write( h, j, l );

	// always matches...
	return 1;
}



__attribute__((hot)) static inline int relay_line( HOST *h, RLINE *l, char *line, int len )
{
	register char *s;

	// length check - don't permit silly lines
	if( len > h->lmax )
	{
		if( ++(h->overlength) >= h->olwarn )
		{
			warn( "Had %lu overlength lines from host %s",
				h->overlength, h->net->name );
			while( h->overlength >= h->olwarn )
				h->olwarn <<= 1;
		}
		return 1;
	}

	l->line = line;
	l->len  = len;

	// we can handle ministry or statsd format
	// relies on statsd format not containing a space
	if( !( s = memchr( line, ' ', len ) )
	 && !( s = memchr( line, ':', len ) ) )
		return 1;

	l->plen = s - line;
	l->sep  = *s;
	*s = '\0';

	++(h->lines);

	return 0;
}

__attribute__((hot)) static inline void relay_dispatch( HOST *h, RLINE *l )
{
	HBUFS *hb;
	RELAY *r;

	for( hb = ((RDATA *) h->data)->hbufs; hb; hb = hb->next )
	{
		r = hb->rule;

		if( (*(r->rfp))( hb, l ) )
		{
			++(r->lines);
			++(h->handled);

			if( r->last )
				break;
		}
	}
}


__attribute__((hot)) void relay_simple( HOST *h, char *line, int len )
{
	RLINE l;

	if( relay_line( h, &l, line, len ) == 0 )
	{
		relay_dispatch( h, &l );
	}
}


__attribute__((hot)) void relay_prefix( HOST *h, char *line, int len )
{
	RLINE l;

	if( relay_line( h, &l, line, len ) == 0 )
	{
		// copy the path onto the end of the prefix
		memcpy( h->ltarget, line, len );
		h->ltarget[len] = '\0';

		// fix l's params - start, and lengths
		l.line  = h->workbuf;
		l.plen += h->plen;
		l.len  += h->plen;

		relay_dispatch( h, &l );
	}
}



// parse the lines
// put any partial lines back at the start of the buffer
// and return the length, if any
__attribute__((hot)) int relay_parse_buf( HOST *h, IOBUF *b )
{
	register char *s = b->bf->buf;
	register char *q;
	int len, l;
	char *r;

	// can't parse without a handler function
	// and those live on the host object
	if( !h )
		return 0;

	len = b->bf->len;

	while( len > 0 )
	{
		// look for newlines
		if( !( q = memchr( s, LINE_SEPARATOR, len ) ) )
		{
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
			++s;
			--l;
		}

		// get the length
		// and trailing \r's
		if( l > 0 && *r == '\r' )
		{
			*r-- = '\0';
			--l;
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

	strbuf_keep( b->bf, len );
	return len;
}


