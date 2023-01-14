/**************************************************************************
* Copyright 2015 John Denholm                                             *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
*                                                                         *
*                                                                         *
* iplist/iplist.c - handles network ip filtering                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

IPL_CTL *_iplist = NULL;


int iplist_add_entry( IPLIST *l, int act, char *str, int len )
{
	IPNET *ip;
	int ret;

	// try to parse it
	if( !( ip = iplist_parse_spec( str, len ) ) )
		return -1;

	ip->act = act;

	// and add it in
	ret = iplist_append_net( l, &ip );

	return ( ret < 0 ) ? -1 : 0;
}


void iplist_add( IPLIST *l, int dup )
{
	l->nets = (IPNET *) mem_reverse_list( l->nets );

	if( dup )
	{
		IPLIST *nl = (IPLIST *) allocz( sizeof( IPLIST ) );

		// grab all the member elements and allocz
		*nl = *l;

		// and send that list back empty
		memset( l, 0, sizeof( IPLIST ) );

		l = nl;
	}

	lock_iplists( );

	l->next = _iplist->lists;
	_iplist->lists = l;
	++(_iplist->lcount);

	unlock_iplists( );
}




IPL_CTL *iplist_config_defaults( void )
{
	char buf[2048];
	IPLIST *lo;
	int rv;

	// make our ip network regex
	_iplist = (IPL_CTL *) mem_perm( sizeof( IPL_CTL ) );
	_iplist->netrgx = (regex_t *) mem_perm( sizeof( regex_t ) );

	pthread_mutex_init( &(_iplist->lock), NULL );

	if( ( rv = regcomp( _iplist->netrgx, IPLIST_REGEX_NET, REG_EXTENDED|REG_ICASE ) ) )
	{
		regerror( rv, _iplist->netrgx, buf, 2048 );
		fatal( "Cannot make IP address regex -- %s", buf );
		return NULL;
	}

	// create a default, localhost-only iplist
	lo = iplist_create( IPLIST_LOCALONLY, IPLIST_NEGATIVE, 3 );
	lo->verbose = 0;
	snprintf( buf, 2048, "127.0.0.1" );
	iplist_add_entry( lo, IPLIST_POSITIVE, buf, 9 );
	iplist_set_text( lo, "Matches only localhost.", 0 );
	iplist_add( lo, 0 );

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
		l->err_dup = 1;
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
		l->name = av_copy( av );
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
	else if( attIs( "match" ) )
	{
		_iplist_cfg_set = 1;
		return iplist_add_entry( l, IPLIST_POSITIVE, av->vptr, av->vlen );
	}
	else if( attIs( "miss" ) || attIs( "unmatch" ) )
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
		iplist_add( &_iplist_cfg_tmp, 1 );
	}
	else
		return -1;

	return 0;
}

