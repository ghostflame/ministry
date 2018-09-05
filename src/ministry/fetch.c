/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* fetch.c - handles fetching metrics                                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "ministry.h"


void fetch_metrics( void *arg, IOBUF *b )
{
	info( "Called fetch_metrics for %d bytes.", b->len );

	// flatten the buffer for now
	b->len = 0;
}


void fetch_ministry( void *arg, IOBUF *b )
{
	FETCH *f = (FETCH *) arg;

	info( "Called fetch_ministry for %d bytes.", b->len );

	data_parse_buf( f->host, b );
}



void fetch_single( int64_t tval, void *arg )
{
	FETCH *f = (FETCH *) arg;

	info( "Fetching for target %s", f->name );

	// this repeatedly calls it's callback with chunks of data
	if( curlw_fetch( f->ch ) != 0 )
		err( "Failed to fetch url: %s", f->ch->url );
}





int fetch_cmp_attrs( const void *ap1, const void *ap2 )
{
	METMP *m1 = *((METMP **) ap1);
	METMP *m2 = *((METMP **) ap2);

	return ( m1->order < m2->order ) ? 1 : ( m1->order == m2->order ) ? 0 : -1;
}


void fetch_sort_attrs( FETCH *f )
{
	METMP **list, *m;
	int i = 0, j;

	if( !f->attct )
		return;

	list = (METMP **) allocz( f->attct * sizeof( METMP * ) );
	for( m = f->attmap; m; m = m->next )
		list[i++] = m;

	qsort( list, f->attct, sizeof( METMP * ), fetch_cmp_attrs );

	// re-link them
	j = f->attct - 1;
	for( i = 0; i < j; i++ )
		list[i]->next = list[i+1];
	list[j]->next = NULL;

	free( list );
}


void fetch_make_url( FETCH *f )
{
	char urlbuf[4096];
	int l;

	l = snprintf( urlbuf, 4096, "http%s://%s:%hu%s",
		( chkCurlF( f->ch, SSL ) ) ? "s" : "",
		f->remote, f->port, f->path );

	f->ch->url = perm_str( l + 1 );
	memcpy( f->ch->url, urlbuf, l );
	f->ch->url[l] = '\0';
}



void *fetch_loop( void *arg )
{
	struct sockaddr_in sin;
	FETCH *f;
	THRD *t;

	t = (THRD *) arg;
	f = (FETCH *) t->arg;
	// we don't use this again
	free( t );

	// clip the offset to the size of the period
	if( f->offset >= f->period )
	{
		warn( "Fetch %s offset larger than period - clipping it.", f->name );
		f->offset = f->offset % f->period;
	}

	// set our timing params, msec -> usec
	f->period *= 1000;
	f->offset *= 1000;

	// sort the attrs
	if( f->metrics && f->attmap )
		fetch_sort_attrs( f );

	// default ports
	if( f->port == 0 )
		f->port = ( chkCurlF( f->ch, SSL ) ) ? 443 : 80;

	// default path
	if( !f->path )
		f->path = str_copy( ( f->metrics ) ? "/metrics" : "/", 0 );

	// and construct the url
	fetch_make_url( f );

	// resolve the IP address
	if( net_lookup_host( f->remote, &sin ) )
		return NULL;	// abort this thread

	// make the host structure
	if( !( f->host = mem_new_host( &sin, (uint32_t) f->bufsz ) ) )
	{
		fatal( "Could not allocate buffer memory or host structure for fetch %s.", f->name );
		return NULL;
	}

	// it's the in side that has the buffer on the host
	f->ch->iobuf = f->host->net->in;

	// metrics or ministry data?
	if( f->metrics )
		f->ch->cb = &fetch_metrics;
	else
		f->ch->cb = &fetch_ministry;

	// and place ourself as the argument
	f->ch->arg = f;

	// set up that host
	f->host->ip   = sin.sin_addr.s_addr;

	if( f->metrics )
		f->dtype = data_type_defns + DATA_TYPE_ADDER;

	f->host->type = f->dtype->nt;

	// we might have prefixes
	if( net_set_host_parser( f->host, 0, 1 ) )
	{
		mem_free_host( &(f->host) );
		return NULL;
	}

	// and run the loop
	loop_control( "fetch", &fetch_single, f, f->period, LOOP_SYNC, f->offset );
	return NULL;
}



int fetch_init( void )
{
	int i = 0;
	FETCH *f;

	// fix the order, for what it matters
	ctl->fetch->targets = mem_reverse_list( ctl->fetch->targets );

	for( i = 0, f = ctl->fetch->targets; f; i++, f = f->next )
		thread_throw( fetch_loop, f, i );

	return ctl->fetch->fcount;
}




int fetch_add_attr( FETCH *f, char *str, int len )
{
	METMP tmp, *m;
	char *cl;

	memset( &tmp, 0, sizeof( METMP ) );

	// were we given an order?
	if( ( cl = memchr( str, ':', len ) ) )
	{
		*cl++ = '\0';
		len -= cl - str;
		tmp.order = atoi( str );
		str = cl;
	}

	if( !len )
	{
		err( "Empty attribute name in fetch block %s.", f->name );
		return -1;
	}

	tmp.attr = str_dup( str, len );
	tmp.alen = len;

	m = (METMP *) allocz( sizeof( METMP ) );
	memcpy( m, &tmp, sizeof( METMP ) );

	m->next = f->attmap;
	f->attmap = m;
	f->attct++;

	return 0;
}


FTCH_CTL *fetch_config_defaults( void )
{
	FTCH_CTL *fc = (FTCH_CTL *) allocz( sizeof( FTCH_CTL ) );
	return fc;
}


static FETCH __fetch_config_tmp;
static int __fetch_config_state = 0;

int fetch_config_line( AVP *av )
{
	FETCH *n, *f = &__fetch_config_tmp;
	int64_t v;
	DTYPE *d;
	int i;

	if( !__fetch_config_state )
	{
		if( f->name )
			free( f->name );
		memset( f, 0, sizeof( FETCH ) );
		f->name = str_copy( "as-yet-unnamed", 0 );
		f->ch = (CURLWH *) allocz( sizeof( CURLWH ) );
		f->bufsz = DEFAULT_FETCH_BUF_SZ;

		// set validate by default if we are given it on
		// the command line.  It principally affects config
		// fetch, but we use it here too
		if( chkcfFlag( SEC_VALIDATE ) )
			setCurlF( f->ch, VALIDATE );
	}

	if( attIs( "name" ) )
	{
		free( f->name );
		f->name = str_copy( av->val, av->vlen );
		__fetch_config_state = 1;
	}
	else if( attIs( "host" ) )
	{
		if( f->remote )
		{
			warn( "Fetch block %s already has a remote host: '%s'", f->name, f->remote );
			free( f->remote );
		}

		f->remote = str_copy( av->val, av->vlen );
		__fetch_config_state = 1;
	}
	else if( attIs( "port" ) )
	{
		if( parse_number( av->val, &v, NULL ) == NUM_INVALID )
		{
			err( "Invalid remote port for fetch block %s: '%s'", f->name, av->val );
			return -1;
		}

		f->port = (uint16_t) v;
		__fetch_config_state = 1;
	}
	else if( attIs( "path" ) )
	{
		if( f->path )
		{
			warn( "Fetch block %s already had a path.", f->name );
			free( f->path );
		}

		if( av->val[0] == '/' )
			f->path = str_copy( av->val, av->vlen );
		else
		{
			f->path = (char *) allocz( av->vlen + 2 );
			memcpy( f->path + 1, av->val, av->vlen );
			f->path[0] = '/';
		}
		__fetch_config_state = 1;
	}
	else if( attIs( "ssl" ) )
	{
		if( config_bool( av ) )
			setCurlF( f->ch, SSL );
		else
			cutCurlF( f->ch, SSL );
	}
	else if( attIs( "validate" ) )
	{
		if( config_bool( av ) )
			setCurlF( f->ch, VALIDATE );
		else
			cutCurlF( f->ch, VALIDATE );
		// absent means use default
	}
	else if( attIs( "prometheus" ) )
	{
		f->metrics = config_bool( av );
	}
	else if( attIs( "type" ) )
	{
		f->dtype = NULL;

		for( i = 0, d = (DTYPE *) data_type_defns; i < DATA_TYPE_MAX; i++, d++ )
			if( !strcasecmp( d->name, av->val ) )
			{
				f->dtype = d;
				break;
			}

		if( !f->dtype )
		{
			err( "Type '%s' not recognised.", av->val );
			return -1;
		}
		if( f->metrics )
			warn( "Data type set to '%s' but this is a prometheus source.", f->dtype->name );
	}
	else if( attIs( "period" ) )
	{
		if( parse_number( av->val, &v, NULL ) == NUM_INVALID )
		{
			err( "Invalid fetch period in block %s: %s", f->name, av->val );
			return -1;
		}

		f->period = v;
	}
	else if( attIs( "offset" ) )
	{
		if( parse_number( av->val, &v, NULL ) == NUM_INVALID )
		{
			err( "Invalid fetch offset in block %s: %s", f->name, av->val );
			return -1;
		}

		f->offset = v;
	}
	else if( attIs( "buffer" ) )
	{
		if( parse_number( av->val, &v, NULL ) == NUM_INVALID )
		{
			err( "Invalid buffer size for block %s: %s", f->name, av->val );
			return -1;
		}

		// cheating - low values are bit shift values
		if( v < 28 )
			v = 1 << v;

		if( v < 0x4000 )	// 16k minimum
		{
			warn( "Fetch block %s buffer is too small at %ld bytes - minimum 16k", f->name, v );
			v = 0x4000;
		}

		f->bufsz = v;
	}
	else if( attIs( "attribute" ) )
	{
		// prometheus attribute map
		return fetch_add_attr( f, av->val, av->vlen );
	}
	else if( attIs( "done" ) )
	{
		if( !f->remote )
		{
			err( "Fetch block %s has no target host!", f->name );
			return -1;
		}
		if( !f->metrics && !f->dtype )
		{
			err( "Fetch block %s has no type!", f->name );
			return -1;
		}

		// copy it
		n = (FETCH *) allocz( sizeof( FETCH ) );
		memcpy( n, f, sizeof( FETCH ) );

		// zero the static struct
		memset( f, 0, sizeof( FETCH ) );
		__fetch_config_state = 0;

		// link it up
		n->next = ctl->fetch->targets;
		ctl->fetch->targets = n;
		ctl->fetch->fcount++;
	}
	else
		return -1;

	return 0;
}
