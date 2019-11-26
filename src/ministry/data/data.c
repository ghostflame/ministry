/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* data/data.c - handles connections and processes stats lines             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"




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
		++(h->invalid);
		return;
	}

	if( !plen )
		return;  // probably a keepalive

	// copy the line next to the prefix
	memcpy( h->ltarget, line, plen );
	plen += h->plen;
	h->ltarget[plen] = '\0';

	if( __data_line_compat_dispatch( h->workbuf, plen, data, *type ) < 0 )
		++(h->invalid);
	else
		++(h->lines);
}



// support the statsd format
// path:<val>|<c or ms>
__attribute__((hot)) void data_line_compat( HOST *h, char *line, int len )
{
	char *data = NULL, *type = NULL;
	int plen;

	if( ( plen = __data_line_compat_check( line, len, &data, &type ) ) < 0 )
	{
		++(h->invalid);
		return;
	}

	if( !plen )
		return;  // probably a keepalive

	if( __data_line_compat_dispatch( line, plen, data, *type ) < 0 )
		++(h->invalid);
	else
		++(h->lines);
}



__attribute__((hot)) void data_line_min_prefix( HOST *h, char *line, int len )
{
	char *ep = NULL;
	int plen;

	if( ( plen = __data_line_ministry_check( line, len, &ep ) ) < 0 || plen > h->lmax )
	{
		++(h->invalid);
		return;
	}

	if( !plen )
		return;  // probably a keepalive

	// looks OK
	++(h->lines);

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
		++(h->invalid);
		return;
	}

	if( !plen )
		return;  // probably a keepalive

	// looks OK
	++(h->lines);

	// and put that in
	(*(h->handler))( line, plen, ep );
}






// parse the lines
// put any partial lines back at the start of the buffer
// and return the length, if any
__attribute__((hot)) int data_parse_buf( HOST *h, IOBUF *b )
{
	register char *s = b->bf->buf;
	register char *q;
	register int l;
	int len;
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
			// partial last line
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
			*r = '\0';
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


void data_fetch_cb( void *arg, IOBUF *b )
{
	FETCH *f = (FETCH *) arg;
	data_parse_buf( f->host, b );
}

