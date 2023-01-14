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
* slack/slack.c - implements a slack client                               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"

SLK_CTL *_slk = NULL;




void slack_add_space( SLKSP *sp )
{
	SLKSP *s;

	// de-dup
	for( s = _slk->spaces; s; s = s->next )
		if( !strcasecmp( sp->name, s->name ) )
		{
			warn( "Freeing duplicate space %s.", sp->name );
			free( sp->name );
			free( sp );
			return;
		}

	sp->next = _slk->spaces;
	_slk->spaces = sp;
}


void slack_channel_clean( SLKCH *ch )
{
	if( ch->name )
		free( ch->name );
	if( ch->spacename )
		free( ch->spacename );
	if( ch->hookurl )
		free( ch->hookurl );
	if( ch->emoji )
		free( ch->emoji );

	memset( ch, 0, sizeof( SLKCH ) );
}


void slack_add_channel( SLKCH *ch )
{
	SLKCH *c;

	// de-dup
	for( c = _slk->cfg_ch_list; c; c = c->next )
		if( !strcasecmp( c->name, ch->name )
		 && !strcasecmp( c->spacename, ch->spacename ) )
		{
			warn( "Freeing duplicate channel %s:%s.", ch->spacename, ch->name );
			slack_channel_clean( ch );
			free( ch );
			return;
		}

	ch->next = _slk->cfg_ch_list;
	_slk->cfg_ch_list = ch;
}



void slack_handle_clean( SLKHD *hd )
{
	SLKPT *p;
	int i;

	for( i = 0; i < LOG_LEVEL_MAX; ++i )
		if( ( p = hd->paths[i] ) )
		{
			if( p->emoji )
				free( p->emoji );
			free( p->spacename );
			free( p->channame );
			free( p );
		}

	if( hd->name )
		free( hd->name );

	memset( hd, 0, sizeof( SLKHD ) );
}


void slack_add_handle( SLKHD *hd )
{
	SLKHD *h;

	for( h = _slk->cfg_hd_list; h; h = h->next )
		if( !strcasecmp( h->name, hd->name ) )
		{
			warn( "Freeing duplicate handle %s.", hd->name );
			slack_handle_clean( hd );
			free( hd );
			return;
		}

	hd->next = _slk->cfg_hd_list;
	_slk->cfg_hd_list = hd;
}






SLK_CTL *slack_config_defaults( void )
{
	_slk = (SLK_CTL *) mem_perm( sizeof( SLK_CTL ) );
	_slk->enabled = 0;
	_slk->handles = string_store_create( MEM_HSZ_NANO, NULL, NULL, 0 );
	_slk->assign  = string_store_create( MEM_HSZ_NANO, NULL, NULL, 0 );

	return _slk;
}


static SLKCH __slk_cfg_chan;
static int __slk_cfg_ch_state = 0;

static SLKHD __slk_cfg_hndl;
static int __slk_cfg_hd_state = 0;

static SLKSP __slk_cfg_spce;
static int __slk_cfg_sp_state = 0;

#define _s_inter	{ err( "Slack channel, space or handle config interleaved." ); return -1; }
#define _s_ch_chk	if( __slk_cfg_hd_state || __slk_cfg_sp_state ) _s_inter
#define _s_hd_chk	if( __slk_cfg_ch_state || __slk_cfg_sp_state ) _s_inter
#define _s_sp_chk	if( __slk_cfg_ch_state || __slk_cfg_hd_state ) _s_inter


int slack_config_line( AVP *av )
{
	SLKCH *nc, *c = &__slk_cfg_chan;
	SLKHD *nh, *h = &__slk_cfg_hndl;
	SLKSP *ns, *s = &__slk_cfg_spce;
	SLKPT *pt;
	SSTE *e;
	WORDS w;
	char *d;
	int i;

	if( !__slk_cfg_ch_state )
	{
		slack_channel_clean( c );
	}
	if( !__slk_cfg_hd_state )
	{
		slack_handle_clean( h );
		memset( h, 0, sizeof( SLKHD ) );
	}
	if( !__slk_cfg_sp_state )
	{
		if( s->name )
			free( s->name );
		memset( s, 0, sizeof( SLKSP ) );
	}

	if( !( d = memchr( av->aptr, '.', av->alen ) ) )
	{
		if( attIs( "enable" ) )
		{
			_slk->enabled = config_bool( av );
		}
		else
			return -1;

		return 0;
	}

	++d;

	if( !strncasecmp( av->aptr, "space.", 6 ) )
	{
		av->aptr += 6;
		av->alen -= 6;

		_s_sp_chk;

		if( attIs( "name" ) )
		{
			if( s->name )
			{
				err( "Space %s has existing name.", s->name );
				return -1;
			}
			s->name = av_copyp( av );
			__slk_cfg_sp_state = 1;
		}
		else if( attIs( "level" ) )
		{
			s->level = log_get_level( av->vptr );
			s->lset  = 1;

			__slk_cfg_sp_state = 1;
		}
		else if( attIs( "done" ) )
		{
			ns = (SLKSP *) mem_perm( sizeof( SLKSP ) );
			*ns = *s;
			memset( s, 0, sizeof( SLKSP ) );
			slack_add_space( ns );
			__slk_cfg_sp_state = 0;
		}
		else
			return -1;
	}
	else if( !strncasecmp( av->aptr, "channel.", 8 ) )
	{
		av->aptr += 8;
		av->alen -= 8;

		_s_ch_chk;

		if( attIs( "space" ) )
		{
			if( c->spacename )
			{
				err( "Channel %s has existing spacename: %s",
					( c->name ) ? c->name : "(unnamed)",
					c->spacename );
				return -1;
			}
			c->spacename = av_copyp( av );
			__slk_cfg_ch_state = 1;
		}
		else if( attIs( "name" ) )
		{
			if( c->name )
			{
				err( "Channel has existing name: %s", c->name );
				return -1;
			}
			c->name = av_copyp( av );
			__slk_cfg_ch_state = 1;
		}
		else if( attIs( "hook") || attIs( "url" ) )
		{
			if( c->hookurl )
			{
				err( "Channel %s has existing webhook url: %s",
					( c->name ) ? c->name : "(unnamed)",					
					c->hookurl );
				return -1;
			}
			c->hookurl = av_copyp( av );
			__slk_cfg_ch_state = 1;
		}
		else if( attIs( "emoji" ) )
		{
			if( c->emoji )
			{
				err( "Channel %s has existing default emoji: %s",
					( c->name ) ? c->name : "(unnamed)",
					c->emoji );
				return -1;
			}
			// there may be :: around the emoji
			// trim them off, but put them back :-)
			if( av->vptr[0] == ':' )
			{
				av->vptr++;
				av->vlen--;
			}
			if( !av->vlen )
			{
				err( "Channel %s has invalid emoji.",
					( c->name ) ? c->name : "(unnamed)" );
				return -1;
			}
			if( av->vptr[av->vlen - 1] == ':' )
			{
				av->vlen--;
				av->vptr[av->vlen] = '\0';
			}

			c->emoji = (char *) allocz( av->vlen + 3 );
			snprintf( c->emoji, av->vlen + 3, ":%s:", av->vptr );

			__slk_cfg_ch_state = 1;
		}
		else if( attIs( "done" ) )
		{
			if( !c->name )
			{
				err( "Channel provided without a name." );
				return -1;
			}
			if( !c->spacename )
			{
				err( "Channel %s provided without a space.", c->name );
				return -1;
			}

			if( !c->hookurl )
			{
				err( "Channel %s:%s provided without a webhook.", c->spacename, c->name );
				return -1;
			}

			nc = (SLKCH *) mem_perm( sizeof( SLKCH ) );
			*nc = *c;
			memset( c, 0, sizeof( SLKCH ) );

			slack_add_channel( nc );

			__slk_cfg_ch_state = 0;
		}
		else
			return -1;
	}
	else if( !strncasecmp( av->aptr, "handle.", 7 ) )
	{
		av->aptr += 7;
		av->alen -= 7;

		_s_hd_chk;

		if( attIs( "name" ) )
		{
			if( h->name )
			{
				err( "Handle %s already has a name.", h->name );
				return -1;
			}
			h->name = av_copyp( av );
			__slk_cfg_hd_state = 1;
		}
		else if( attIs( "done" ) )
		{
			if( !h->name )
			{
				err( "Handle provided without a name." );
				return -1;
			}

			if( !h->pct )
			{
				err( "Handle provided without any pathways." );
				return -1;
			}


			nh = (SLKHD *) mem_perm( sizeof( SLKHD ) );
			*nh = *h;
			memset( h, 0, sizeof( SLKHD ) );

			slack_add_handle( nh );

			__slk_cfg_hd_state = 0;
		}
		else
		{
			for( i = LOG_LEVEL_MIN; i < LOG_LEVEL_MAX; ++i )
				if( attIs( log_get_level_name( i ) ) )
				{
					// we are expecting space:channel[:emoji]
					strwords( &w, av->vptr, av->vlen, ':' );
					if( w.wc < 2 )
					{
						err( "Invalid pathway details: %s", av->vptr );
						return -1;
					}

					pt = (SLKPT *) mem_perm( sizeof( SLKPT ) );
					pt->spacename = str_perm( w.wd[0], w.len[0] );
					pt->channame = str_perm( w.wd[1], w.len[1] );
					if( w.wc > 2 && w.len[2] > 0 )
					{
						pt->emoji = mem_perm( w.len[2] + 3 );
						snprintf( pt->emoji, w.len[2] + 3, ":%s:", w.wd[2] );
					}

					h->paths[i] = pt;
					++(h->pct);

					return 0;
				}

			return -1;
		}
	}
	else if( !strncasecmp( av->aptr, "assign.", 7 ) )
	{
		av->aptr += 7;
		av->alen -= 7;

		if( av->alen == 0 )
			return -1;

		// anything we like, really
		e = string_store_add( _slk->assign, av->aptr, av->alen );
		e->ptr = av_copyp( av );
	}
	else
		return -1;

	return 0;
}
