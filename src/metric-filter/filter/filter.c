/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* filter/filter.c - filtering functions                                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"




__attribute__((hot)) void filter_flush_host( HOST *h, int refresh )
{
	if( !h->net->out->bf->len )
		return;

	lock_host( h );

	target_write_all( h->net->out );

	if( refresh )
	{
		h->net->out = mem_new_iobuf( IO_BUF_SZ );
		h->net->out->expires = ctl->proc->curr_tval + ctl->filt->flush_max;
	}

	unlock_host( h );
}



__attribute__((hot)) int filter_host_line( HFILT *hf )
{
	FILT *f;

	if( hf->best_mode == FILTER_MODE_ALL )
		return 0;

	for( f = hf->filters; f; f = f->next )
		if( regex_list_test( hf->path->buf, f->matches ) == REGEX_MATCH )
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
			// look for the space, or a colon, for statsd format
		  	if( !( p = memchr( s, ' ', l ) ) )
				p = memchr( s, ':', l );

			// stomp on the first space
			if( p )
			{
				strbuf_copymax( hf->path, s, p - s );
				++(h->lines);		

				// does it pass muster?
				if( filter_host_line( hf ) == 0 )
				{
					++(h->handled);

					// and forward that line
					if( !buf_hasspace( o->bf, l + 1 ) )
						filter_flush_host( h, 1 );

					buf_appends( o->bf, s, l );
					buf_addchar( o->bf, '\n' );
				}
			}
		}

		// and move on
		s = q;
	}

	if( b->bf->len > 0 )
		filter_flush_host( h, 1 );

	strbuf_keep( b->bf, len );
	return len;
}



void filter_host_watcher( THRD *t )
{
	int64_t us;
	HFILT *hf;
	HOST *h;

	hf = (HFILT *) t->arg;
	h  = (HOST *) hf->host;

	// half, convert to usec
	us = ctl->filt->flush_max / 2000;

	while( hf->running && RUNNING( ) )
	{
		usleep( us );

		lock_host( h );

		if( hf->running && ctl->proc->curr_tval > h->net->out->expires )
		{
			if( h->net->out->bf->len > 0 )
			{
				target_write_all( h->net->out );
				h->net->out = mem_new_iobuf( IO_BUF_SZ );
				h->net->out->expires = ctl->proc->curr_tval + ctl->filt->flush_max;
			}
			else
				h->net->out->expires = ctl->proc->curr_tval + ctl->filt->flush_max;
		}

		unlock_host( h );

		// shut this host if the generation of filters changes
		if( hf->gen && hf->gen < ctl->filt->generation )
			flagf_add( h->net, IO_CLOSE );
	}

	t->arg = NULL;

	// sanity time
	usleep( 1000000 );
	mem_free_hfilt( &hf );
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

	flist = mem_new_hfilt( );
	flist->best_mode = FILTER_MODE_MAX;
	flist->running = 1;
	flist->host = h;

	if( _filt->close_conn )
		flist->gen = ctl->filt->generation;

	// we need locking on this one
	secure_host( h );

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

	    // name the thread after the type and remote host
    thread_throw_named_f( &filter_host_watcher, flist, 0, "w_%08x:%04x",
        ntohl( h->net->peer.sin_addr.s_addr ),
        ntohs( h->net->peer.sin_port ) );
}


void filter_host_end( HOST *h )
{
	HFILT *f;

	f = (HFILT *) h->data;

	lock_host( h );

	if( f )
		f->running = 0;

	// note, do not free the hfilt struct -
	h->data = NULL;

	if( h->net->out->bf->len > 0 )
		filter_flush_host( h, 0 );
	else
		mem_free_iobuf( &(h->net->out) );

	unlock_host( h );
}



