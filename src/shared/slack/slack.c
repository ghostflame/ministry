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

		json_insert( jo, k, string, v );
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


