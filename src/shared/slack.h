/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* slack.h - functions handling comms with slack                           *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#ifndef SHARED_SLACK_H
#define SHARED_SLACK_H


#define SLACK_MSG_BUF_SZ			0xffff		// 64k
#define SLACK_CURL_FLAGS			CURLW_FLAG_SEND_JSON|CURLW_FLAG_VERBOSE|CURLW_FLAG_DEBUG|CURLW_FLAG_PARSE_JSON




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


#define json_fetch( _obj, _key, _type )			json_object_get_##_type( json_object_object_get( _obj, _key ) )
#define json_insert( _obj, _key, _type, _item )	json_object_object_add( _obj, _key, json_object_new_##_type( _item ) )


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
