/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* fetch/fetch.c - handles fetching metrics                                *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "ministry.h"




void fetch_single( int64_t tval, void *arg )
{
	FETCH *f = (FETCH *) arg;

	if( !f->ready )
	{
		debug( "Fetch target %s is not ready (no DNS).", f->name );
		return;
	}

	//info( "Fetching for target %s", f->name );

	// this repeatedly calls it's callback with chunks of data
	if( curlw_fetch( f->ch, NULL, 0 ) != 0 )
		err( "Failed to fetch url: %s", f->ch->url );
}


// called regularly
void fetch_check_dns( int64_t tval, void *arg )
{
	FETCH *f = (FETCH *) arg;
	struct sockaddr_in sin;

	memset( &sin, 0, sizeof( struct sockaddr_in ) );

	// resolve the IP address
	if( net_lookup_host( f->remote, &sin ) )
		f->ready = 0;
	else
	{
		f->ready = 1;
		sin.sin_port = htons( f->port );
	}

	// and set that
	if( f->host )
	{
		io_sock_set_peer( f->host->net, &sin );
		f->host->ip = sin.sin_addr.s_addr;
	}
}




void fetch_make_url( FETCH *f )
{
	char urlbuf[4096];
	int l;

	l = snprintf( urlbuf, 4096, "http%s://%s:%hu%s",
		( chkCurlF( f->ch, SSL ) ) ? "s" : "",
		f->remote, f->port, f->path );

	f->ch->url = str_perm( urlbuf, l );

	// we might get json
	setCurlF( f->ch, PARSE_JSON );
}



void fetch_revalidate_loop( THRD *t )
{
	FETCH *f = (FETCH *) t->arg;

	loop_control( "fetch_dns", &fetch_check_dns, f, f->revalidate, LOOP_TRIM, f->revalidate );
}



void fetch_loop( THRD *t )
{
	FETCH *f = (FETCH *) t->arg;
	struct sockaddr_in sin;

	// default the period to the adder period
	if( !f->period )
	{
		info( "Fetch %s defaulting to 10s." );
		f->period = 10000;
	}

	// clip the offset to the size of the period
	if( f->offset >= f->period )
	{
		warn( "Fetch %s offset larger than period - clipping it.", f->name );
		f->offset = f->offset % f->period;
	}

	// set our timing params, msec -> usec
	f->period *= 1000;
	f->offset *= 1000;

	// default ports
	if( f->port == 0 )
		f->port = ( chkCurlF( f->ch, SSL ) ) ? 443 : 80;

	// default path
	if( !f->path )
		f->path = str_copy( ( f->metrics ) ? "/metrics" : "/", 0 );

	// and construct the url
	fetch_make_url( f );

	// make the host structure
	memset( &sin, 0, sizeof( struct sockaddr_in ) );
	if( !( f->host = mem_new_host( &sin, (uint32_t) f->bufsz ) ) )
	{
		fatal( "Could not allocate buffer memory or host structure for fetch %s.", f->name );
		return;
	}

	// and try looking up the IP address
	fetch_check_dns( 0, f );

	// did we get an answer?
	if( !f->ready && !f->revalidate )
		return;	// abort this thread then

	// it's the in side that has the buffer on the host
	f->ch->iobuf = f->host->net->in;

	// and place ourself as the argument
	f->ch->arg = f;

	// set up prometheus metrics bits
	if( f->metrics )
	{
		f->ch->cb = &metrics_fetch_cb;
		f->metdata->profile_name = f->profile;

		metrics_init_data( f->metdata );
	}
	// and regular ministry data parser
	else
	{
		f->host->type = f->dtype->nt;

		// we might have prefixes
		if( net_set_host_parser( f->host, 0, 1 ) )
		{
			mem_free_host( &(f->host) );
			return;
		}

		f->ch->cb  = &data_fetch_cb;
		f->ch->jcb = &data_fetch_jcb;
	}

	// we may well have got 0-ttl dns, eg in Kubernetes, or from consul
	if( f->revalidate )
	{
		f->revalidate *= 1000;
		thread_throw_named_f( fetch_revalidate_loop, f, t->id, "fetch_dns_%d", t->id );
	}

	// and run the loop
	loop_control( "fetch", &fetch_single, f, f->period, LOOP_SYNC, f->offset );
	return;
}



int fetch_init( void )
{
	FETCH *f;
	int i;

	// fix the order, for what it matters
	ctl->fetch->targets = mem_reverse_list( ctl->fetch->targets );

	for( i = 0, f = ctl->fetch->targets; f; f = f->next )
		if( f->enabled )
		{
			thread_throw_named_f( fetch_loop, f, i, "fetch_loop_%d", i );
			++i;
		}

	return ctl->fetch->fcount;
}


