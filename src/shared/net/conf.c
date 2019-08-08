/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* net/conf.c - handles network configuration                              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

NET_CTL *_net = NULL;


uint32_t net_masks[33] = {
	0x00000000,
	0x00000001,
	0x00000003,
	0x00000007,
	0x0000000f,
	0x0000001f,
	0x0000003f,
	0x0000007f,
	0x000000ff,
	0x000001ff,
	0x000003ff,
	0x000007ff,
	0x00000fff,
	0x00001fff,
	0x00003fff,
	0x00007fff,
	0x0000ffff,
	0x0001ffff,
	0x0003ffff,
	0x0007ffff,
	0x000fffff,
	0x001fffff,
	0x003fffff,
	0x007fffff,
	0x00ffffff,
	0x01ffffff,
	0x03ffffff,
	0x07ffffff,
	0x0fffffff,
	0x1fffffff,
	0x3fffffff,
	0x7fffffff,
	0xffffffff
};


int net_add_type( NET_TYPE *nt )
{
	if( !nt )
		return -1;

	nt->nlen = strlen( nt->name );
	nt->next = _net->ntypes;
	_net->ntypes = nt;

	_net->ntcount++;

	return 0;
}


void net_host_callbacks( tcp_fn *setup, tcp_fn *finish )
{
	_net->host_setup  = setup;
	_net->host_finish = finish;
}



NET_CTL *net_config_defaults( void )
{
	NET_CTL *net;

	net              = (NET_CTL *) allocz( sizeof( NET_CTL ) );
	net->dead_time   = NET_DEAD_CONN_TIMER;
	net->rcv_tmout   = NET_RCV_TMOUT;

	// create our tokens structure
	net->tokens      = token_setup( );

	return net;
}


#define ntflag( f )			if( config_bool( av ) ) nt->flags |= NTYPE_##f; else nt->flags &= ~NTYPE_##f



int net_add_prefixes( char *name, int len )
{
	NET_PFX *p;
	WORDS w;

	strmwords( &w, name, len, ' ' );

	if( w.wc != 2 )
	{
		err( "Invalid prefixes spec; format is: <list name> <prefix>" );
		return -1;
	}

	// duplicates against one list will mask one of them
	for( p = _net->prefix; p; p = p->next )
		if( !strcasecmp( p->name, w.wd[0] ) )
		{
			err( "Duplicate prefix list name reference (%s).", p->name );
			return -1;
		}

	p = (NET_PFX *) allocz( sizeof( NET_PFX ) );
	p->name = str_dup( w.wd[0], w.len[0] );
	p->text = str_dup( w.wd[1], w.len[1] );

	p->next = _net->prefix;
	_net->prefix = p;

	return 0;
}



int net_config_line( AVP *av )
{
	struct in_addr ina;
	char *d, *cp;
	NET_TYPE *nt;
	int i, tcp;
	int64_t v;
	WORDS *w;

	if( !( d = strchr( av->aptr, '.' ) ) )
	{
		if( attIs( "timeout" ) )
		{
			av_int( v );
			_net->dead_time = (time_t) v;
			debug( "Dead connection timeout set to %d sec.", _net->dead_time );
		}
		else if( attIs( "rcvTmout" ) )
		{
			av_int( v );
			_net->rcv_tmout = (unsigned int) v;
			debug( "Receive timeout set to %u sec.", _net->rcv_tmout );
		}
		else if( attIs( "prefix" ) )
		{
			return net_add_prefixes( av->vptr, av->vlen );
		}
		else if( attIs( "filterList" ) )
		{
			_net->filter_list = str_copy( av->vptr, av->vlen );
		}
		else
			return -1;

		return 0;
	}

	/* then it's data., statsd. or adder. (or ipcheck) */
	d++;



	if( !strncasecmp( av->aptr, "tokens.", 7 ) )
	{
		av->alen -= 7;
		av->aptr += 7;

		if( attIs( "enable" ) )
			_net->tokens->enable = config_bool( av );
		else if( attIs( "msec" ) || attIs( "lifetime" ) )
		{
			i = atoi( av->vptr );
			if( i < 10 )
			{
				i = DEFAULT_TOKEN_LIFETIME;
				warn( "Minimum token lifetime is 10msec - setting to %d", i );
			}
			_net->tokens->lifetime = i;
		}
		else if( attIs( "hashsize" ) )
		{
			if( !( _net->tokens->hsize = hash_size( av->vptr ) ) )
				return -1;
		}
		else if( attIs( "mask" ) )
		{
			av_int( _net->tokens->mask );
		}
		else if( attIs( "filter" ) )
		{
			if( _net->tokens->filter_name )
				free( _net->tokens->filter_name );

			_net->tokens->filter_name = str_copy( av->vptr, av->vlen );
		}
		else
			return -1;

		return 0;
	}
	else
	{
		// go looking for the net type
		for( nt = _net->ntypes; nt; nt = nt->next )
			if( !strncasecmp( av->aptr, nt->name, nt->nlen ) && av->aptr[nt->nlen] == '.' )
				break;

		if( !nt )
			return -1;
	}

	av->alen -= nt->nlen + 1;
	av->aptr += nt->nlen + 1;

	// only a few single-words per connection type
	if( !strchr( av->aptr, '.' ) )
	{
		if( attIs( "enable" ) )
		{
			ntflag( ENABLED );
			debug( "Networking %s for %s", ( nt->flags & NTYPE_ENABLED ) ? "enabled" : "disabled", nt->label );
		}
		else if( attIs( "label" ) )
		{
			free( nt->label );
			nt->label = strdup( av->vptr );
		}
		else
			return -1;

		return 0;
	}

	// which port struct are we working with?
	if( !strncasecmp( av->aptr, "udp.", 4 ) )
		tcp = 0;
	else if( !strncasecmp( av->aptr, "tcp.", 4 ) )
		tcp = 1;
	else
		return -1;

	av->alen -= 4;
	av->aptr += 4;

	if( attIs( "enable" ) )
	{
		if( tcp )
		{
			ntflag( TCP_ENABLED );
		}
		else
		{
			ntflag( UDP_ENABLED );
		}
	}
	else if( attIs( "threads" ) )
	{
		if( !tcp )
			warn( "Threads is only for TCP connections - there is one thread per UDP port." );
		else
		{
			if( parse_number( av->vptr, &v, NULL ) == NUM_INVALID )
			{
				err( "Invalid TCP thread count: %s", av->vptr );
				return -1;
			}
			if( v < 1 || v > 1024 )
			{
				err( "TCP thread count must be 1 <= X <= 1024." );
				return -1;
			}

			nt->threads = v;
		}
	}
	else if( attIs( "pollMax" ) )
	{
		if( !tcp )
			warn( "Pollmax is only for TCP connections." );
		else
		{
			if( parse_number( av->vptr, &v, NULL ) == NUM_INVALID )
			{
				err( "Invalid TCP pollmax count: %s", av->vptr );
				return -1;
			}
			if( v < 1 || v > 1024 )
			{
				err( "TCP pollmax must be 1 <= X <= 1024." );
				return -1;
			}

			nt->pollmax = v;
		}
	}
	else if( attIs( "style" ) )
	{
		if( tcp )
		{
			// do we recognise it?
			for( i = 0; i < TCP_STYLE_MAX; i++ )
				if( !strcasecmp( av->vptr, tcp_styles[i].name ) )
				{
					debug( "TCP handling style set to %s", av->vptr );
					nt->tcp_style = tcp_styles[i].style;
					return 0;
				}

			err( "TCP style '%s' not recognised.", av->vptr );
			return -1;
		}
		else
			warn( "Cannot set a handling style on UDP handling." );
	}
	else if( attIs( "checks" ) )
	{
		if( tcp )
			warn( "To disable prefix checks on TCP, set enable 0 on prefixes." );
		else
		{
			ntflag( UDP_CHECKS );
		}
	}
	else if( attIs( "bind" ) )
	{
		inet_aton( av->vptr, &ina );
		if( tcp )
			nt->tcp->ip = ina.s_addr;
		else
			nt->udp_bind = ina.s_addr;
	}
	else if( attIs( "backlog" ) )
	{
		if( tcp )
		{
			av_int( v );
			nt->tcp->back = (unsigned short) v;
		}
		else
			warn( "Cannot set a backlog for a UDP connection." );
	}
	else if( attIs( "port" ) || attIs( "ports" ) )
	{
		if( tcp )
		{
			av_int( v );
			nt->tcp->port = (unsigned short) v;
		}
		else
		{
			// work out how many ports we have
			w  = (WORDS *) allocz( sizeof( WORDS ) );
			cp = strdup( av->vptr );
			strwords( w, cp, 0, ',' );
			if( w->wc > 0 )
			{
				nt->udp_count = w->wc;
				nt->udp       = (NET_PORT **) allocz( w->wc * sizeof( NET_PORT * ) );

				debug( "Discovered %d udp ports for %s", w->wc, nt->label );

				for( i = 0; i < w->wc; i++ )
				{
					nt->udp[i]       = (NET_PORT *) allocz( sizeof( NET_PORT ) );
					nt->udp[i]->port = (unsigned short) strtoul( w->wd[i], NULL, 10 );
					nt->udp[i]->type = nt;
				}
			}

			free( cp );
			free( w );
		}
	}
	else
		return -1;

	return 0;
}

#undef ntflag
