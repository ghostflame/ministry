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
* slack/slack.h - functions handling comms with slack                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#ifndef SHARED_SLACK_H
#define SHARED_SLACK_H

#define SLACK_MSG_BUF_SZ			0xffff		// 64k


struct slack_message
{
	SLKMSG			*	next;
	BUF				*	text;
	char			*	line;
	JSON			*	jo;
};




struct slack_channel
{
	SLKCH			*	next;
	SLKSP			*	space;

	char			*	spacename;
	char			*	name;
	char			*	hookurl;
	char			*	emoji;

	CURLWH			*	ch;
};


struct slack_pathway
{
	SLKPT			*	next;

	SLKSP			*	space;
	SLKCH			*	channel;

	char			*	spacename;
	char			*	channame;
	char			*	emoji;
};


struct slack_handle
{
	SLKHD			*	next;

	char			*	name;
	SLKPT			*	paths[LOG_LEVEL_MAX];

	int					pct;
};



struct slack_space
{
	SLKSP			*	next;

	char			*	name;
	SSTR			*	channels;
	int8_t				level;
	int8_t				lset;
};


struct slack_control
{
	SLKCH			*	cfg_ch_list;
	SLKHD			*	cfg_hd_list;

	SLKSP			*	spaces;
	SSTR			*	handles;
	int					enabled;

	SSTR			*	assign;
};



void slack_message_set_icon_emoji( SLKMSG *msg, char *str );					// set the emoji
int slack_message_add_attachment( SLKMSG *msg, int argc, ... );					// add keys/values in pairs (strings only)
SLKMSG *slack_message_create( char *fmt, ... );									// create a message
int slack_message_simple( SLKHD *hdl, int8_t level, char *fmt, ... );			// create and send and clear
int slack_message_send( SLKHD *hdl, int8_t level, SLKMSG *msg, int clear );		// if clear == 0, the message is kept

SLKCH *slack_find_channel( SLKSP *space, char *name );
SLKHD *slack_find_handle( char *name );
SLKHD *slack_find_assignment( char *key );
SLKSP *slack_find_space( char *name );


int slack_init( void );

SLK_CTL *slack_config_defaults( void );
conf_line_fn slack_config_line;

#endif
