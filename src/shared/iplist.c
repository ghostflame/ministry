/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* iplist.c - handles network ip filtering                                 *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"

IPL_CTL *_iplist = NULL;

// TESTING

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
	IPLIST *l = NULL;

	if( name && *name )
		for( l = _iplist->lists; l; l = l->next )
			if( !strcmp( name, l->name ) )
				break;

	if( !l )
		err( "Cannot find filter list named '%s'", name );

	return l;
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

	for( i = 0; i < l->hashsz; i++ )
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
	for( i = 0; i < l->hashsz; i++ )
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
			bits = (int8_t) atoi( str + rm[6].rm_so );
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



int iplist_append_net( IPLIST *l, IPNET *n )
{
	IPNET *i = NULL, **list;
	uint32_t hval;
	int add = 0;

	n->list = l;

	// hosts are different - check for dupes
	if( n->bits == 32 )
	{
		if( !l->ips )
		{
			if( !l->hashsz )
				l->hashsz = IPLIST_HASHSZ;

			l->ips = (IPNET **) allocz( l->hashsz * sizeof( IPNET * ) );
		}

		hval = n->net % l->hashsz;

		for( i = l->ips[hval]; i; i = i->next )
			if( i->net == n->net )
				break;

		if( !i )
		{
			add = 1;
			list = &(l->ips[hval]);
		}
	}
	else
	{
		for( i = l->nets; i; i = i->next )
			if( i->net == n->net && i->bits == n->bits )
				break;

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
		l->count++;
		return 0;
	}

	err( "Duplicate IP check rule: %s", n->name );
	return -1;
}


int iplist_add_entry( IPLIST *l, int act, char *str, int len )
{
	IPNET *ip;

	// try to parse it
	if( !( ip = iplist_parse_spec( str, len ) ) )
		return -1;

	ip->act = act;

	// and add it in
	return iplist_append_net( l, ip );
}


void iplist_add( IPLIST *l )
{
	l->nets = (IPNET *) mem_reverse_list( l->nets );

	l->next = _iplist->lists;
	_iplist->lists = l;
	_iplist->lcount++;
}




void iplist_init( void )
{
	uint32_t i;
	IPLIST *l;
	IPNET *n;

	for( l = _iplist->lists; l; l = l->next )
	{
		if( l->text )
		{

			for( n = l->nets; n; n = n->next )
				if( !n->text )
				{
					n->text = l->text;
					n->tlen = l->tlen;
				}

			for( i = 0; i < l->hashsz; i++ )
				for( n = l->ips[i]; n; n = n->next )
					if( !n->text )
					{
						n->text = l->text;
						n->tlen = l->tlen;
					}
		}

		l->init_done = 1;
	}
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




IPL_CTL *iplist_config_defaults( void )
{
	char buf[2048];
	IPLIST *lo;
	int rv;

	// make our ip network regex
	_iplist = (IPL_CTL *) allocz( sizeof( IPL_CTL ) );
	_iplist->netrgx = (regex_t *) allocz( sizeof( regex_t ) );

	if( ( rv = regcomp( _iplist->netrgx, IPLIST_REGEX_NET, REG_EXTENDED|REG_ICASE ) ) )
	{
		regerror( rv, _iplist->netrgx, buf, 2048 );
		fatal( "Cannot make IP address regex -- %s", buf );
		return NULL;
	}

	// create a default, localhost-only iplist
	lo = (IPLIST *) allocz( sizeof( IPLIST ) );
	lo->name = IPLIST_LOCALONLY;
	lo->def = IPLIST_NEGATIVE;
	lo->verbose = 0;
	lo->hashsz = 3;
	snprintf( buf, 2048, "127.0.0.1" );
	iplist_add_entry( lo, IPLIST_POSITIVE, buf, 9 );
	iplist_set_text( lo, "Matches only localhost.", 0 );
	iplist_add( lo );

	return _iplist;
}



static IPLIST _iplist_cfg_tmp;
static int    _iplist_cfg_set = 0;

// handle iplist parsing here
int iplist_config_line( AVP *av )
{
	IPLIST *l = &_iplist_cfg_tmp;
	int32_t hs;

	if( !_iplist_cfg_set )
	{
		memset( l, 0, sizeof( IPLIST ) );
	}

	if( attIs( "enable" ) )
	{
		l->enable = config_bool( av );
		_iplist_cfg_set = 1;
	}
	else if( attIs( "name" ) )
	{
		if( l->name )
			free( l->name );
		l->name = str_copy( av->vptr, av->vlen );
		_iplist_cfg_set = 1;
	}
	else if( attIs( "default" ) )
	{
		l->def = config_bool( av ) ? IPLIST_POSITIVE : IPLIST_NEGATIVE;
		_iplist_cfg_set = 1;
	}
	else if( attIs( "verbose" ) )
	{
		l->verbose = config_bool( av );
		_iplist_cfg_set = 1;
	}
	else if( attIs( "hashsize" ) )
	{
		hs = (uint32_t) hash_size( av->vptr );
		if( l->ips )
			warn( "Hashsize %d ignored - already used default (%d).  Set it before adding entries for it to take effect.", hs, l->hashsz );
		else
			l->hashsz = hs;
		_iplist_cfg_set = 1;
	}
	else if( attIs( "match" ) || attIs( "whitelist" ) )
	{
		_iplist_cfg_set = 1;
		return iplist_add_entry( l, IPLIST_POSITIVE, av->vptr, av->vlen );
	}
	else if( attIs( "miss" ) || attIs( "blacklist" ) )
	{
		_iplist_cfg_set = 1;
		return iplist_add_entry( l, IPLIST_NEGATIVE, av->vptr, av->vlen );
	}
	else if( attIs( "entry" ) )
	{
		_iplist_cfg_set = 1;
		return iplist_add_entry( l, IPLIST_NEITHER, av->vptr, av->vlen );
	}
	else if( attIs( "text" ) )
	{
		_iplist_cfg_set = 1;
		iplist_set_text( l, av->vptr, av->vlen );
	}
	else if( attIs( "done" ) )
	{
		if( !_iplist_cfg_set || !l->name || !l->count )
		{
			err( "Iplist not fully configured, cannot add." );
			return -1;
		}

		_iplist_cfg_set = 0;
		iplist_add( &_iplist_cfg_tmp );
	}
	else
		return -1;

	return 0;
}

