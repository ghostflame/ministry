/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* relay/conf.c - relay configuration functions                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"


RLY_CTL *_relay = NULL;



RLY_CTL *relay_config_defaults( void )
{
	RLY_CTL *r = (RLY_CTL *) allocz( sizeof( RLY_CTL ) );
	NET_TYPE *nt;

	nt               = (NET_TYPE *) allocz( sizeof( NET_TYPE ) );
	nt->tcp          = (NET_PORT *) allocz( sizeof( NET_PORT ) );
	nt->tcp->ip      = INADDR_ANY;
	nt->tcp->back    = DEFAULT_NET_BACKLOG;
	nt->tcp->port    = DEFAULT_RELAY_PORT;
	nt->tcp->type    = nt;
	nt->tcp_style    = TCP_STYLE_THRD;
	nt->flat_parser  = &relay_simple;
	nt->prfx_parser  = &relay_prefix;
	nt->buf_parser   = &relay_parse_buf;
	nt->udp_bind     = INADDR_ANY;
	nt->label        = strdup( "relay" );
	nt->name         = strdup( "relay" );
	nt->flags        = NTYPE_ENABLED|NTYPE_TCP_ENABLED|NTYPE_UDP_ENABLED;

	net_add_type( nt );

	r->flush_nsec    = MILLION * RELAY_FLUSH_MSEC;
	r->net           = nt;

	_relay = r;

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
		if( parse_number( av->vptr, &ms, NULL ) == NUM_INVALID )
		{
			err( "Invalid flush milliseconds value '%s'", av->vptr );
			return -1;
		}
		_relay->flush_nsec = MILLION * ms;

		if( _relay->flush_nsec <= 0 )
			_relay->flush_nsec = MILLION * RELAY_FLUSH_MSEC;
	}
	// this is the other way of setting the flush times
	else if( attIs( "realtime" ) )
	{
		if( config_bool( av ) )
			_relay->flush_nsec = MILLION * RELAY_FLUSH_MSEC_FAST;
		else
			_relay->flush_nsec = MILLION * RELAY_FLUSH_MSEC_SLOW;
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
			++s;
		}

		if( regcomp( r->matches + r->mcount, s, REG_EXTENDED|REG_ICASE|REG_NOSUB ) )
		{
			err( "Invalid regex for relay block '%s': '%s'",
				r->name, av->vptr );
			return -1;
		}

		// keep a copy of the string
		r->rgxstr[r->mcount] = str_dup( av->vptr, av->vlen );

		++(r->mcount);
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
		n->next = _relay->rules;
		_relay->rules = n;

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




