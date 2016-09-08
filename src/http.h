/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* http.h - defines HTTP structures and functions                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_HTTP_H
#define MINISTRY_HTTP_H


typedef cb_ContentReader     MHD_ContentReaderCallback;
typedef cb_ContentReaderFree MHD_ContentReaderFreeCallback;
typedef cb_AccessPolicy      MHD_AccessPolicyCallback;
typedef cb_AccessHandler     MHD_AccessHandlerCallback;
typedef cb_RequestCompleted  MHD_RequestCompletedCallback;
typedef it_KeyValue          MHD_KeyValueIterator;
typedef it_PostData          MHD_PostDataIterator;


typedef HTTP_CONN struct MHD_Connection;
typedef HTTP_CODE enum MHD_RequestTerminationCode;


cb_AccessHandler http_request_handler;
cb_RequestCompleted http_request_complete;


struct http_control
{
	MHD_Daemon				*	server;
	cb_ContentReader		*	cb_cr;
	cb_AccessPolicy			*	cb_ap;
	cb_AccessHandler		*	cb_ah;
	cb_RequestCompleted		*	cb_rc;
	it_KeyValue				*	it_kv;
	it_PostData				*	it_pd;

	uint16_t					port;
	int							enabled;
	int							stats;
};






throw_fn http_loop;

HTTP_CTL *http_default_config( void );
int http_config_line( AVP *av );

#endif
