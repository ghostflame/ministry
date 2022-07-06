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
* slack/init.c - init the slack client                                    *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"

void slack_response_cb( void *arg, IOBUF *buf )
{
	SLKCH *ch = (SLKCH *) arg;

	if( buf->bf->len )
		info( "Slack %s:%s replied: %s", ch->name, ch->space->name, buf->bf->buf );
}



SLKSP *slack_find_space( char *name )
{
	SLKSP *s;

	for( s = _slk->spaces; s; s = s->next )
		if( !strcasecmp( name, s->name ) )
			return s;

	return NULL;
}

SLKCH *slack_find_channel( SLKSP *space, char *name )
{
	SSTE *e;

	e = string_store_look( space->channels, name, strlen( name ), 0 );
	return ( e ) ? (SLKCH *) e->ptr : NULL;
}

SLKHD *slack_find_handle( char *name )
{
	SSTE *e;

	e = string_store_look( _slk->handles, name, strlen( name ), 0 );
	return ( e ) ? (SLKHD *) e->ptr : NULL;
}

SLKHD *slack_find_assignment( char *key )
{
	SLKHD *hdl = NULL;
	SSTE *e;

	if( ( e = string_store_look( _slk->assign, key, strlen( key ), 0 ) ) )
		hdl = slack_find_handle( (char *) e->ptr );

	return hdl;
}



// now we have to resolve all those names
int slack_init( void )
{
	int i, sc, cc, hc;
	SLKCH *c;
	SLKSP *s;
	SLKHD *h;
	SLKPT *p;
	SSTE *e;

	if( !_slk->enabled )
		return 0;

	sc = cc = hc = 0;

	// default all spaces to warn and worse
	// keep slack spam to a minimum
	for( s = _slk->spaces; s; s = s->next )
	{
		if( s->lset == 0 )
		{
			s->level = LOG_LEVEL_WARN;
			s->lset = 1;
		}

		// create the lookups
		s->channels = string_store_create( MEM_HSZ_NANO, NULL, NULL, 0 );
		++sc;
	}

	// settle channels into spaces
	while( _slk->cfg_ch_list )
	{
		c = _slk->cfg_ch_list;
		_slk->cfg_ch_list = c->next;

		if( !( c->space = slack_find_space( c->spacename ) ) )
		{
			err( "Could not find slack space '%s' referred to by channel '%s'.",
				c->spacename, c->name );
			return -1;
		}

		c->next = NULL;
		c->ch = curlw_handle( c->hookurl, SLACK_CURL_FLAGS, &slack_response_cb, NULL, c, NULL );

		e = string_store_add( c->space->channels, c->name, strlen( c->name ) );
		e->ptr = c;

		++cc;
	}

	// settle pathways into channels
	while( _slk->cfg_hd_list )
	{
		h = _slk->cfg_hd_list;
		_slk->cfg_hd_list = h->next;

		for( i = 0; i < LOG_LEVEL_MAX; ++i )
		{
			if( !( p = h->paths[i] ) )
				continue;

			if( !( p->space = slack_find_space( p->spacename ) ) )
			{
				err( "Could not find slack space '%s' referred to by pathway.", p->spacename );
				return -1;
			}

			if( !( p->channel = slack_find_channel( p->space, p->channame ) ) )
			{
				err( "Could not find slack channel '%s:%s' referred to by pathway",
					p->space->name, p->channame );
				return -1;
			}
		}

		e = string_store_add( _slk->handles, h->name, strlen( h->name ) );
		e->ptr = h;

		++hc;
	}

	info( "Initialised %d slack space%s, %d channel%s and %d handle%s.",
		sc, plural( sc ), cc, plural( cc ), hc, plural( hc ) );


	// set up one we want
	//_proc->apphdl = slack_find_assignment( "app" );

	return 0;
}



