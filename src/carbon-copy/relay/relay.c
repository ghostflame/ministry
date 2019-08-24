/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* relay/relay.c - handles connections and processes lines                 *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"



__attribute__((hot)) int relay_write( IOBUF *b, RLINE *l )
{
//printf( "[%3d] %s\n",   l->plen, l->path );
//printf( "[%3d] %s\n\n", l->rlen, l->rest );

	// add path
	memcpy( b->buf + b->len, l->path, l->plen );
	b->len += l->plen;

	// and a space
	b->buf[b->len++] = ' ';

	// and the rest
	memcpy( b->buf + b->len, l->rest, l->rlen );
	b->len += l->rlen;

	// and the newline
	b->buf[b->len++] = '\n';

	// and see if we need to write
	if( b->len >= IO_BUF_HWMK )
		return 1;

	return 0;
}



__attribute__((hot)) int relay_regex( HBUFS *h, RLINE *l )
{
	RELAY *r = h->rule;
	uint8_t mat;
	int j;

	// try each regex
	for( j = 0; j < r->mcount; ++j )
	{
		mat  = regexec( r->matches + j, l->path, 0, NULL, 0 ) ? 0 : 1;
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
	if( relay_write( h->bufs[0], l ) )
	{
		io_buf_post( r->targets[0], h->bufs[0] );
		h->bufs[0] = mem_new_iobuf( IO_BUF_SZ );
	}

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
		j = (*(h->rule->hfp))( l->path, l->plen ) % h->rule->tcount;

	// hash writes to just one target
	if( relay_write( h->bufs[j], l ) )
	{
		io_buf_post( h->rule->targets[j], h->bufs[j] );
		h->bufs[j] = mem_new_iobuf( IO_BUF_SZ );
	}

	// always matches...
	return 1;
}





__attribute__((hot)) void relay_simple( HOST *h, char *line, int len )
{
	HBUFS *hb;
	RELAY *r;
	RLINE l;
	char *s;

//printf( "[%3d] %s\n", len, line );

	// we can handle ministry or statsd format
	// relies on statsd format not containing a space
	if( !( s = memchr( line, ' ', len ) )
	 && !( s = memchr( line, ':', len ) ) )
	{
		// not a valid line
		return;
	}

	// we will reconstruct the line later on
	l.path = line;
	l.plen = s - line;
	*s++ = '\0';
	l.rest = s;
	l.rlen = len - l.plen - 1;

	++(h->lines);

	// run through until we get a match and a last
	for( hb = ((RDATA*) h->data)->hbufs; hb; hb = hb->next )
	{
		r = hb->rule;

		if( (*(r->rfp))( hb, &l ) )
		{
			++(r->lines);
			++(h->handled);
			if( r->last )
				break;
		}
	}
}


__attribute__((hot)) void relay_prefix( HOST *h, char *line, int len )
{
	HBUFS *hb;
	RELAY *r;
	RLINE l;
	char *s;

	if( !( s = memchr( line, ' ', len ) ) )
		// not a valid line
		return;

	// we will reconstruct the line later on
	l.path = line;
	l.plen = s - line;
	*s++ = '\0';
	l.rest = s;
	l.rlen = len - l.plen - 1;

	// can't handle a line that long
	if( l.plen >= h->lmax )
	{
		if( ++(h->overlength) >= h->olwarn )
		{
			warn( "Had %lu overlength lines from host %s",
				h->overlength, h->net->name );
			while( h->overlength >= h->olwarn )
				h->olwarn <<= 1;
		}

		// done with that
		return;
	}

	++(h->lines);

	// copy the path onto the end of the prefix
	memcpy( h->ltarget, line, l.plen );
	h->ltarget[l.plen] = '\0';
	l.path = h->workbuf;
	l.plen = l.plen + h->plen;

	// run through using the prefixed 
	for( hb = ((RDATA *) h->data)->hbufs; hb; hb = hb->next )
	{
		r = hb->rule;

		if( (*(r->rfp))( hb, &l ) )
		{
			++(r->lines);
			++(h->handled);
			if( r->last )
				break;
		}
	}
}



// parse the lines
// put any partial lines back at the start of the buffer
// and return the length, if any
__attribute__((hot)) int relay_parse_buf( HOST *h, IOBUF *b )
{
	register char *s = b->buf;
	register char *q;
	int len, l;
	char *r;

	// can't parse without a handler function
	// and those live on the host object
	if( !h )
		return 0;

	len = b->len;

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

	io_buf_keep( b, len );
	return len;
}


