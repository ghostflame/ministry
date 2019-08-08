/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* relay.c - handles connections and processes lines                       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "carbon_copy.h"



__attribute__((hot)) int relay_write( IOBUF *b, RLINE *l )
{
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
	for( j = 0; j < r->mcount; j++ )
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

	h->lines++;

	// run through until we get a match and a last
	for( hb = (HBUFS *) h->data; hb; hb = hb->next )
	{
		r = hb->rule;

		if( (*(r->rfp))( hb, &l ) )
		{
			r->lines++;
			h->handled++;
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

	h->lines++;

	// copy the path onto the end of the prefix
	memcpy( h->ltarget, line, l.plen );
	h->ltarget[l.plen] = '\0';
	l.path = h->workbuf;
	l.plen = l.plen + h->plen;

	// run through using the prefixed 
	for( hb = (HBUFS *) h->data; hb; hb = hb->next )
	{
		r = hb->rule;

		if( (*(r->rfp))( hb, &l ) )
		{
			r->lines++;
			h->handled++;
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
			// partial last line
			l = s - b->buf;

			if( len < l )
			{
				memcpy( b->buf, s, len );
				*(b->buf + len) = '\0';
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


void relay_buf_set( HOST *h )
{
	HBUFS *b, *tmp, *list;
	RELAY *r;
	int i;

	tmp = list = NULL;

	for( r = ctl->relay->rules; r; r = r->next )
	{
		b = mem_new_hbufs( );

		// set the relay function and buffers
		switch( r->type )
		{
			case RTYPE_REGEX:
				b->bufs[0] = mem_new_iobuf( IO_BUF_SZ );
				b->bcount  = 1;
				break;
			case RTYPE_HASH:
				b->bcount = r->tcount;
				for( i = 0; i < r->tcount; i++ )
					b->bufs[i] = mem_new_iobuf( IO_BUF_SZ );
				break;
		}

		b->rule = r;
		b->next = tmp;
		tmp = b;
	}

	// and get the list order correct
	while( tmp )
	{
		b = tmp;
		tmp = b->next;

		b->next = list;
		list = b;
	}

	h->data = (void *) list;
}



int relay_resolve( void )
{
	RELAY *r;
	WORDS w;
	int i;

	for( r = ctl->relay->rules; r; r = r->next )
	{
		// special case - blackhole
		if( !strcasecmp( r->target_str, "blackhole" ) )
		{
			r->drop = 1;
			continue;
		}

		strwords( &w, r->target_str, strlen( r->target_str ), ',' );
		for( i = 0; i < w.wc; i++ )
		{
			if( !target_list_find( w.wd[i] ) )
				return -1;
		}

		r->tcount  = w.wc;
		r->targets = (TGTL **) allocz( w.wc * sizeof( TGTL * ) );

		for( i = 0; i < w.wc; i++ )
			r->targets[i] = target_list_find( w.wd[i] );
	}

	return 0;
}



RLY_CTL *relay_config_defaults( void )
{
	RLY_CTL *r;
	r = (RLY_CTL *) allocz( sizeof( RLY_CTL ) );

	return r;
}

static RELAY __relay_cfg_tmp;
static int __relay_cfg_state = 0;

int relay_config_line( AVP *av )
{
	RELAY *n, *r = &__relay_cfg_tmp;
	int64_t ms;
	char *s;

	if( !__relay_cfg_state )
	{
		memset( r, 0, sizeof( RELAY ) );
		r->last = 1;
		r->type = RTYPE_UNKNOWN;
		r->name = str_copy( "- unnamed -", 0 );
		r->matches = (regex_t *) allocz( RELAY_MAX_REGEXES * sizeof( regex_t ) );
		r->rgxstr  = (char **)   allocz( RELAY_MAX_REGEXES * sizeof( char *  ) );
		r->invert  = (uint8_t *) allocz( RELAY_MAX_REGEXES * sizeof( uint8_t ) );
	}

	if( attIs( "flush" ) || attIs( "flushMsec" ) )
	{
		if( parse_number( av, &ms, NULL ) == NUM_INVALID )
		{
			err( "Invalid flush milliseconds value '%s'", av->vptr );
			return -1;
		}
		ctl->relay->flush_nsec = MILLION * ms;

		if( ctl->relay->flush_nsec <= 0 )
			ctl->relay->flush_nsec = MILLION * NET_FLUSH_MSEC;
	}
	else if( attIs( "name" ) )
	{
		if( r->name && strcmp( r->name, "- unnamed -" ) )
		{
			warn( "Relay block '%s' already had a name.", r->name );
			free( r->name );
		}

		r->name = str_copy( av->vptr, av->vlen );
		__relay_cfg_state = 1;
	}
	else if( attIs( "regex" ) )
	{
		if( r->mcount == RELAY_MAX_REGEXES )
		{
			err( "Relay block '%s' is allowed a maximum of %d matches.",
				r->name, RELAY_MAX_REGEXES );
			return -1;
		}

		s = av->vptr;
		if( *s == '!' )
		{
			r->invert[r->mcount] = 1;
			s++;
		}

		if( regcomp( r->matches + r->mcount, s, REG_EXTENDED|REG_ICASE|REG_NOSUB ) )
		{
			err( "Invalid regex for relay block '%s': '%s'",
				r->name, av->vptr );
			return -1;
		}

		// keep a copy of the string
		r->rgxstr[r->mcount] = str_dup( av->vptr, av->vlen );

		r->mcount++;
		r->type = RTYPE_REGEX;

		__relay_cfg_state = 1;
	}
	else if( attIs( "last" ) )
	{
		r->last = config_bool( av );
		debug( "Relay block '%s' terminates processing on a match.",
			r->name );
		__relay_cfg_state = 1;
	}
	else if( attIs( "continue" ) )
	{
		r->last = !config_bool( av );
		debug( "Relay block '%s' does not terminate processing on a match.",
			r->name );
		__relay_cfg_state = 1;
	}
	else if( attIs( "hash" ) )
	{
		if( r->mcount )
		{
			warn( "Relay block '%s' is being set to type has but has %d regexes.",
				r->name, r->mcount );
		}
		if( valIs( "cksum" ) )
			r->hfp = &hash_cksum;
		else if( valIs( "fnv1" ) )
			r->hfp = &hash_fnv1;
		else if( valIs( "fnv1a" ) )
			r->hfp = &hash_fnv1a;
		else
		{
			err( "Unrecognised hash type: %s", av->vptr );
			return -1;
		}
		r->type = RTYPE_HASH;
		__relay_cfg_state = 1;
	}
	else if( attIs( "target" ) || attIs( "targets" ) )
	{
		r->target_str = str_copy( av->vptr, av->vlen );
		__relay_cfg_state = 1;
	}
	else if( attIs( "done" ) )
	{
		if( !r->name || !strcmp( r->name, "- unnamed -" )
		 || !r->target_str
		 || r->type == RTYPE_UNKNOWN
		 || ( r->mcount == 0 && r->type != RTYPE_HASH ) )
		{
			err( "A relay block needs a name, a target and some regex strings." );
			return -1;
		}

		// let's make a new struct with the right amount of matches
		n = (RELAY *) allocz( sizeof( RELAY ) );

		// copy across what we need
		memcpy( n, r, sizeof( RELAY ) );

		// make it's own memory
		n->mstats  = (int64_t *) allocz( n->mcount * sizeof( int64_t ) );
		n->matches = (regex_t *) allocz( n->mcount * sizeof( regex_t ) );
		n->rgxstr  = (char **)   allocz( n->mcount * sizeof( char *  ) );
		n->invert  = (uint8_t *) allocz( n->mcount * sizeof( uint8_t ) );

		// and copy in the regexes and inverts
		memcpy( n->matches, r->matches, n->mcount * sizeof( regex_t ) );
		memcpy( n->rgxstr,  r->rgxstr,  n->mcount * sizeof( char *  ) );
		memcpy( n->invert,  r->invert,  n->mcount * sizeof( uint8_t ) );

		// set the relay function and buffers
		switch( n->type )
		{
			case RTYPE_REGEX:
				n->rfp  = &relay_regex;
				break;
			case RTYPE_HASH:
				n->rfp  = &relay_hash;
				break;
			default:
				err( "Cannot handle relay type %d.", n->type );
				return -1;
		}

		// and link it in
		n->next = ctl->relay->rules;
		ctl->relay->rules = n;

		debug( "Added relay block '%s' with %d regexes.", n->name, n->mcount );

		// free up the allocated memory
		free( r->matches );
		free( r->rgxstr );
		free( r->invert );
		free( r->name );
		__relay_cfg_state = 0;
	}
	else
		return -1;

	return 0;
}


