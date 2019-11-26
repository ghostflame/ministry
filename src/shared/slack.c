/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* slack.c - implements a slack client                                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "shared.h"

SLK_CTL *_slk = NULL;


void slack_response_cb( void *arg, IOBUF *buf )
{
	SLKCH *ch = (SLKCH *) arg;

	if( buf->bf->len )
		info( "Slack %s:%s replied: %s", ch->name, ch->space->name, buf->bf->buf );
}


void slack_message_set_icon_emoji( SLKMSG *msg, char *str )
{
	char emobuf[64];
	int i;

	if( *str == ':' )
		str++;

	i = snprintf( emobuf, 63, ":%s", str );
	if( emobuf[i] != ':' )
	{
		emobuf[i++] = ':';
		emobuf[i]   = '\0';
	}

	// replace any existing
	if( json_object_object_get( msg->jo, "icon_emoji" ) )
		json_object_object_del( msg->jo, "icon_emoji" );

	json_insert( msg->jo, "icon_emoji", string, emobuf );
}


int slack_message_add_attachment( SLKMSG *msg, int argc, ... )
{
	JSON *ja, *jo;
	va_list args;
	char *k, *v;
	int i;

	if( !msg || !msg->jo )
	{
		warn( "Cannot add attachment - no valid message." );
		return -1;
	}

	if( argc & 0x1 )
	{
		warn( "Cannot add attachment with uneven argument count" );
		return -2;
	}

	// halve argc, as we take them in twos
	argc >>= 1;

	// make sure attachments exists
	if( !( ja = json_object_object_get( msg->jo, "attachments" ) ) )
		json_object_object_add( msg->jo, "attachments", json_object_new_array( ) );

	// and create a member for it
	jo = json_object_new_object( );

	va_start( args, argc );
	for( i = 0; i < argc; ++i )
	{
		k = va_arg( args, char * );
		v = va_arg( args, char * );

		json_object_object_add( jo, k, json_object_new_string( v ) );
	}
	va_end( args );

	json_object_array_put_idx( ja, json_object_array_length( ja ), jo );
	return 0;
}


SLKMSG *__slack_message_creator( char *fmt, va_list args )
{
	SLKMSG *m = mem_new_slack_msg( 0 );

	strbuf_vprintf( m->text, fmt, args );
	m->line = str_copy( m->text->buf, m->text->len );

	// start the object
	m->jo = json_object_new_object( );
	json_insert( m->jo, "text", string, m->text->buf );
	strbuf_empty( m->text );

	return m;
}

SLKMSG *slack_message_create( char *fmt, ... )
{
	va_list args;
	SLKMSG *m;

	va_start( args, fmt );
	m = __slack_message_creator( fmt, args );
	va_end( args );

	return m;
}


int __slack_message_checks( SLKHD *hdl, int8_t level, SLKPT **pt )
{
	if( !_slk->enabled )
		return 1;

	if( !hdl )
		return -1;

	if( level < 0 || level >= LOG_LEVEL_MAX )
		return -1;

	// do we have a defined pathway?
	if( !( *pt = hdl->paths[level] ) )
		return -1;

	// over level?
	if( level > (*pt)->space->level )
		return -1;

	return 0;
}


void  __slack_message_sender( SLKPT *pt, int8_t level, SLKMSG *msg, int clear )
{
	char *emoji = NULL;

	// do we have an icon emoji?
	// if not, go hunting through the pathway and channel
	emoji = (char *) json_fetch( msg->jo, "icon_emoji", string );
	if( !emoji )
	{
		emoji = ( pt->emoji ) ? pt->emoji : pt->channel->emoji;
		if( emoji )
			json_insert( msg->jo, "icon_emoji", string, emoji );
	}

	// convert that json object to text
	strbuf_json( msg->text, msg->jo, clear );

	// now try with content
	curlw_fetch( pt->channel->ch, msg->text->buf, msg->text->len );

	// also log it
	log_line( 0, level, NULL, 0, NULL, msg->line );

	if( clear )
		mem_free_slack_msg( &msg );
	// did we add an emoji?
	else if( emoji )
	{
		// then we should clear it, as it would show up next time
		// the message might get re-used with different emoji
		json_object_object_del( msg->jo, "icon_emoji" );
	}
}


int slack_message_simple( SLKHD *hdl, int8_t level, char *fmt, ... )
{
	va_list args;
	SLKPT *pt;
	SLKMSG *m;

	if( __slack_message_checks( hdl, level, &pt ) != 0 )
		return -1;

	va_start( args, fmt );
	m = __slack_message_creator( fmt, args );
	va_end( args );

	__slack_message_sender( pt, level, m, 1 );
	return 0;
}


int slack_message_send( SLKHD *hdl, int8_t level, SLKMSG *msg, int clear )
{
	SLKPT *pt;

	if( !msg )
	{
		err( "Cannot send NULL message to slack." );
		return -1;
	}

	if( __slack_message_checks( hdl, level, &pt ) != 0 )
		return -1;

	__slack_message_sender( pt, level, msg, clear );
	return 0;
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
	_slk = (SLK_CTL *) allocz( sizeof( SLK_CTL ) );
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
			ns = (SLKSP *) allocz( sizeof( SLKSP ) );
			memcpy( ns, s, sizeof( SLKSP ) );
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

			nc = (SLKCH *) allocz( sizeof( SLKCH ) );
			memcpy( nc, c, sizeof( SLKCH ) );
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


			nh = (SLKHD *) allocz( sizeof( SLKHD ) );
			memcpy( nh, h, sizeof( SLKHD ) );
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

					pt = (SLKPT *) allocz( sizeof( SLKPT ) );
					pt->spacename = str_dup( w.wd[0], w.len[0] );
					pt->channame = str_dup( w.wd[1], w.len[1] );
					if( w.wc > 2 && w.len[2] > 0 )
					{
						pt->emoji = str_perm( w.len[2] + 3 );
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
