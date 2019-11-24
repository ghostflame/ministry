/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* filter/filter.c - filtering functions                                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

#define filter_host_flush( _h, _r )			target_write_all( _h->net->out ); if( _r ) _h->net->out = mem_new_iobuf( IO_BUF_SZ )



__attribute__((hot)) int filter_host_line( HFILT *hf )
{
	FILT *f;
	int r;

	if( hf->best_mode == FILTER_MODE_ALL )
		return 0;

	for( f = hf->filters; f; f = f->next )
	{
		r = regex_list_test( hf->path->buf, f->matches );
		if( r == REGEX_MATCH )
		{
			switch( f->mode )
			{
				case FILTER_MODE_ALLOW:
					return 0;
				case FILTER_MODE_DROP:
					return -1;
				default:
					return 0;
			}
		}
		else
		{
			switch( f->mode )
			{
				case FILTER_MODE_ALLOW:
					return -1;
				case FILTER_MODE_DROP:
					return 0;
				default:
					return -1;
			}
		}
	}

	return -1;
}


__attribute__((hot)) int filter_parse_buf( HOST *h, IOBUF *b )
{
	register char *s = b->bf->buf;
	register char *q;
	register int l;
	char *r, *p;
	HFILT *hf;
	IOBUF *o;
	int len;

	if( !h )
		return 0;

	hf  = (HFILT *) h->data;
	o   = h->net->out;
	len = b->bf->len;

	while( len > 0 )
	{
		// break by newlines
		if( !( q = memchr( s, LINE_SEPARATOR, len ) ) )
		{
			// partial last line
			// we are done with len > 0
			break;
		}

		l = q - s;
		r = q;

		// stomp on that newline
		*q++ = '\0';

		// decrement remaining length
		len -= q - s;

		// clean leading \r's
		if( *s == '\r' )
		{
			++s;
			--l;
		}

		// get the length and trailing \r's
		if( l > 0 && *r == '\r' )
		{
			*r = '\0';
			--l;
		}

		// still got anything?
		if( l > 0 )
		{
			// stomp on the first space
			if( ( p = memchr( s, ' ', l ) ) )
			{
				strbuf_copymax( hf->path, s, p - s );
				++(h->lines);		

				// does it pass muster?
				if( filter_host_line( hf ) == 0 )
				{
					++(h->handled);

					// and forward that line
					if( !buf_hasspace( o->bf, l + 1 ) )
					{
						filter_host_flush( h, 1 );
					}

					buf_appends( o->bf, s, l );
					buf_addchar( o->bf, '\n' );
				}
			}
		}

		// and move on
		s = q;
	}

	if( b->bf->len > 0 )
	{
		filter_host_flush( h, 1 );
	}

	strbuf_keep( b->bf, len );
	return len;
}


// figure out which IPNETs this host matches
// grab the filter data object from each one
// and copy it
void filter_host_setup( HOST *h )
{
	MEMHL *matches, *nlist;
	MEMHG *hg, *nhg;
	HFILT *flist;
	FILT *f, *fn;
	IPNET *nd;

	matches = mem_list_create( 0, NULL );
	if( iplist_test_ip_all( _filt->fconf->ipl, h->ip, matches ) == IPLIST_NOMATCH )
	{
		// this host is not allowed to send anything
		flagf_add( h->net, IO_CLOSE );
		mem_list_free( matches );
		return;
	}

	flist = (HFILT *) allocz( sizeof( HFILT ) );
	flist->path = strbuf( MAX_PATH_SZ );

	for( hg = matches->head; hg; hg = hg->next )
	{
		nd = (IPNET *) hg->ptr;
		nlist = (MEMHL *) nd->data;

		mem_list_lock( nlist );

		for( nhg = nlist->head; nhg; nhg = nhg->next )
		{
			fn  = (FILT *) allocz( sizeof( FILT ) );
			f   = (FILT *) nhg->ptr;
			*fn = *f;

			fn->next = flist->filters;
			flist->filters = fn;

			if( fn->mode < flist->best_mode )
				flist->best_mode = fn->mode;
		}

		mem_list_unlock( nlist );
	}

	// make a socket for outgoing data
	h->net->out = mem_new_iobuf( NET_BUF_SZ );

	mem_list_free( matches );
	h->data = flist;
}

void filter_host_end( HOST *h )
{
	FILT *f, *fn;
	HFILT *hf;

	hf = (HFILT *) h->data;
	for( f = hf->filters; f; f = fn )
	{
		fn = f->next;
		free( f );
	}
	strbuf_free( hf->path );
	free( hf );
	h->data = NULL;

	if( h->net->out->bf->len > 0 )
	{
		filter_host_flush( h, 0 );
	}
	else
		mem_free_iobuf( &(h->net->out) );
}



