/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* iplist/iplist.c - handles network ip filtering                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


// TESTING

// gather up all matching ips/networks
// used by metric filter
int iplist_test_ip_all( IPLIST *l, uint32_t ip, MEMHL *matches )
{
	IPNET *n;

	if( !matches )
		return IPLIST_NOMATCH;

	if( !l || !l->enable || !l->count )
		return IPLIST_NOMATCH;

	// hash first
	for( n = l->ips[ip % l->hashsz]; n; n = n->next )
		if( ip == n->net && n->act == IPLIST_POSITIVE )
			mem_list_add_tail( matches, n );

	// then networks
	for( n = l->nets; n; n = n->next )
		if( n->act == IPLIST_POSITIVE && ( ip & net_masks[n->bits] ) == n->net )
			mem_list_add_tail( matches, n );

	return ( mem_list_size( matches ) ) ? IPLIST_POSITIVE : IPLIST_NOMATCH;
}


// test yes/no for matching
// used in the general network accept/drop case
int iplist_test_ip( IPLIST *l, uint32_t ip, IPNET **p )
{
	IPNET *n;

	if( p )
		*p = NULL;

	if( !l || !l->enable || !l->count )
		return IPLIST_NOMATCH;

	// hash first
	for( n = l->ips[ip % l->hashsz]; n; n = n->next )
		if( ip == n->net )
		{
			if( p )
				*p = n;

			return n->act;
		}

	// then the networks
	for( n = l->nets; n; n = n->next )
		if( ( ip & net_masks[n->bits] ) == n->net )
		{
			if( p )
				*p = n;

			return n->act;
		}

	return IPLIST_NOMATCH;
}


int iplist_test_str( IPLIST *l, char *ip, IPNET **p )
{
	struct in_addr ina;

	if( !ip || !inet_aton( ip, &ina ) )
		return IPLIST_NOMATCH;

	return iplist_test_ip( l, ina.s_addr, p );
}





// FINDING

IPLIST *iplist_find( char *name )
{
	IPLIST *l;

	if( name && *name )
		for( l = _iplist->lists; l; l = l->next )
			if( !strcmp( name, l->name ) )
				return l;

	err( "Cannot find filter list named '%s'", name );
	return NULL;
}


// in case you want to attach structures to the IPNETs
// eg prefixing structures
// this allows you to call a callback on each configured item
void iplist_call_data( IPLIST *l, iplist_data_fn *fp, void *arg )
{
	uint32_t i;
	IPNET *n;

	if( !l )
		return;

	for( n = l->nets; n; n = n->next )
		(*(fp))( arg, n );

	for( i = 0; i < l->hashsz; ++i )
		for( n = l->ips[i]; n; n = n->next )
			(*(fp))( arg, n );
}


void __iplist_explain_list( IPNET *list, char *pos, char *neg, char *nei, char *pre, char *fmt )
{
	char *s, *t;
	IPNET *n;

	for( n = list; n; n = n->next )
	{
		switch( n->act )
		{
			case IPLIST_NEGATIVE:
				s = neg;
				break;
			case IPLIST_POSITIVE:
				s = pos;
				break;
			case IPLIST_NEITHER:
				s = nei;
				break;
			default:
				s = "";
				break;
		}
		t = ( n->text && n->tlen ) ? n->text : "";

		info( fmt, pre, s, n->name, t );
	}
}

void iplist_explain( IPLIST *l, char *pos, char *neg, char *nei, char *pre )
{
	int len, sz = 0;
	char fmt[64];
	uint32_t i;

	if( !l )
		return;

	if( !pos )
		pos = "Allow";
	if( !neg )
		neg = "Deny";
	if( !nei )
		nei = "Unknown";
	if( !pre )
		pre = "    ";

	len = strlen( pos );
	if( len > sz )
		sz = len;
	len = strlen( neg );
	if( len > sz )
		sz = len;
	len = strlen( nei );
	if( len > sz )
		sz = len;

	// make a format that fits our words
	snprintf( fmt, 64, "%%s%%-%ds%%22s    %%s", sz );

	info( "IP list %s: ", l->name );
	for( i = 0; i < l->hashsz; ++i )
		__iplist_explain_list( l->ips[i], pos, neg, nei, pre, fmt );

	__iplist_explain_list( l->nets, pos, neg, nei, pre, fmt );
}


// SETUP


IPNET *iplist_parse_spec( char *str, int len )
{
	struct sockaddr_in sa;
	struct in_addr ina;
	char *sp, buf[32];
	regmatch_t rm[7];
	IPNET *n = NULL;
	int bits = -1;

	if( ( sp = memchr( str, ' ', len ) ) )
	{
		while( *sp == ' ' )
			*sp++ = '\0';

		len -= sp - str;
	}
	else
		len = 0;

	if( !regexec( _iplist->netrgx, str, 7, rm, 0 ) )
	{
		// cap the IP address - stomp on the /
		// or it just might be the end of the string
		str[rm[1].rm_eo] = '\0';

		if( rm[6].rm_so > -1 )
			bits = (int8_t) strtol( str + rm[6].rm_so, NULL, 10 );
		else
			bits = 32;

		if( bits < 0 || bits > 32 )
		{
			err( "Invalid network bits %d", bits );
			return NULL;
		}

		// is that IP ok?
		if( !inet_aton( str, &(sa.sin_addr) ) )
		{
			err( "Invalid IP address %s", str );
			return NULL;
		}
	}
	else
	{
		if( net_lookup_host( str, &sa ) )
			return NULL;

		bits = 32;
	}

	if( bits > -1 )
	{
		n = (IPNET *) allocz( sizeof( IPNET ) );
		n->bits = bits;

		// grab the network
		// this makes 172.16.10.20/16 work as a spec
		n->net  = sa.sin_addr.s_addr & net_masks[bits];

		// make a permanent copy of the config parameter
		if( sp && len )
		{
			n->text = str_dup( sp, len );
			n->tlen = len;
		}

		ina.s_addr = n->net;
		snprintf( buf, 32, "%s/%hhd", inet_ntoa( ina ), n->bits );
		n->name = str_dup( buf, 0 );
	}

	return n;
}


void iplist_free_net( IPNET *n )
{
	if( !n )
		return;

	if( n->name )
		free( n->name );

	if( n->text )
		free( n->next );

	free( n );
}


void iplist_free_list( IPLIST *l )
{
	IPNET *n, *list;
	uint32_t i;

	if( l->ips )
	{
		for( i = 0; i < l->hashsz; ++i )
		{
			list = l->ips[i];
			while( list )
			{
				n = list;
				list = n->next;
				iplist_free_net( n );
			}
		}
		free( l->ips );
	}

	list = l->nets;
	while( list )
	{
		n = list;
		list = n->next;
		iplist_free_net( n );
	}

	if( l->text )
		free( l->text );

	if( l->name )
		free( l->name );

	free( l );
}



int iplist_append_net( IPLIST *l, IPNET **p )
{
	IPNET *i = NULL, *n, **list;
	int add = 0;

	if( !( n = *p ) )
		return -1;

	n->list = l;

	// hosts are different - check for dupes
	if( n->bits == 32 )
	{
		// grumble grumble O(N^2)
		// no way to do this without
		// requiring hash size earlier
		for( i = l->singles; i; i = i->next )
			if( i->net == n->net )
			{
				*p = i;
				break;
			}

		if( !i )
		{
			// actually hash it into position later
			add = 1;
			list = &(l->singles);
		}
	}
	else
	{
		for( i = l->nets; i; i = i->next )
			if( i->net == n->net && i->bits == n->bits )
			{
				*p = i;
				break;
			}

		if( !i )
		{
		  	add = 1;
			list = &(l->nets);
		}
	}

	if( add )
	{
		n->next = *list;
		*list = n;

		if( n->text )
			debug( "Added IP rule %s --> %s", n->name, n->text );
		else
		{
			debug( "Added ip rule %s --> %s", n->name,
				( n->act == IPLIST_POSITIVE ) ? "positive" :
				( n->act == IPLIST_NEGATIVE ) ? "negative" : "neither" );
		}
		++(l->count);
		return 0;
	}

	iplist_free_net( n );

	if( l->err_dup )
	{
		err( "Duplicate IP check rule: %s", (*p)->name );
		return -1;
	}

	return 1;
}


void iplist_init_one( IPLIST *l )
{
	uint32_t hval, i;
	IPNET *n;

	if( l->init_done )
		return;

	if( !l->hashsz )
		l->hashsz = IPLIST_HASHSZ;

	l->ips = (IPNET **) allocz( l->hashsz * sizeof( IPNET * ) );

	// these have been de-dup'd already
	while( l->singles )
	{
		n = l->singles;
		l->singles = n->next;

		hval = n->net % l->hashsz;

		n->next = l->ips[hval];
		l->ips[hval] = n;
	}

	if( l->text )
	{
		for( n = l->nets; n; n = n->next )
			if( !n->text )
			{
				n->text = l->text;
				n->tlen = l->tlen;
			}

		for( i = 0; i < l->hashsz; ++i )
			for( n = l->ips[i]; n; n = n->next )
				if( !n->text )
				{
					n->text = l->text;
					n->tlen = l->tlen;
				}
	}

	l->init_done = 1;
}


// called at startup
// won't catch the IPlists loaded by metric-filter
void iplist_init( void )
{
	IPLIST *l;

	for( l = _iplist->lists; l; l = l->next )
		iplist_init_one( l );
}


int iplist_set_text( IPLIST *l, char *str, int len )
{
	if( !l )
	{
		warn( "No iplist provided." );
		return -1;
	}

	if( l->init_done )
	{
		warn( "Cannot set text on iplist %s - it has been init'd.",
			l->name );
		return -2;
	}

	if( l->text )
	{
		warn( "Text already set on list %s: '%s'",
			( l->name ) ? l->name : "unknown", l->text );
		free( l->text );
		l->tlen = 0;
	}

	if( str && !len )
		len = strlen( str );

	if( str )
		l->text = str_copy( str, len );

	return 0;
}





IPLIST *iplist_create( char *name, int default_return, int hashsz )
{
	IPLIST *l = (IPLIST *) allocz( sizeof( IPLIST ) );

	if( name )
		l->name = str_copy( name, 0 );

	l->def = default_return;
	l->hashsz = hashsz;

	return l;
}


