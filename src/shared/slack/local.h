/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* slack/local.h - private functions handling comms with slack             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#ifndef SHARED_SLACK_LOCAL_H
#define SHARED_SLACK_LOCAL_H


#define SLACK_CURL_FLAGS			CURLW_FLAG_SEND_JSON|CURLW_FLAG_VERBOSE|CURLW_FLAG_DEBUG|CURLW_FLAG_PARSE_JSON


#include "shared.h"


extern SLK_CTL *_slk;

#endif
