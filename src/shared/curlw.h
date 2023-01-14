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
* curlw.h - defines curl wrapper functions                                *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_CURLW_H
#define SHARED_CURLW_H

// curl options check
// can't use verify below 7.41.0
#if LIBCURL_VERSION_MAJOR > 7 || \
  ( LIBCURL_VERSION_MAJOR == 7 && \
    LIBCURL_VERSION_MINOR > 40 )
	#define _LCURL_CAN_VERIFY 1
#else
	#define _LCURL_CAN_VERIFY 0
#endif

// in case we have an old version of curl
#ifndef CURLOPT_SSL_VERIFYPEER
#define CURLOPT_SSL_VERIFYPEER		64L
#endif

#ifndef CURLOPT_SSL_VERIFYHOST
#define CURLOPT_SSL_VERIFYHOST		81L
#endif

#ifndef CURLOPT_SSL_VERIFYSTATUS
#define CURLOPT_SSL_VERIFYSTATUS	232L
#endif



#define CErr						curl_easy_strerror( cc )

#define CURLW_FLAG_SSL				0x0001
#define CURLW_FLAG_VALIDATE			0x0002
#define CURLW_FLAG_VERBOSE			0x0008
#define CURLW_FLAG_SLOW				0x0010
#define CURLW_FLAG_TOFILE			0x0020
#define CURLW_FLAG_PARSE_JSON		0x0040
#define CURLW_FLAG_TIMINGS			0x0080
#define CURLW_FLAG_SEND_JSON		0x0100
#define CURLW_FLAG_DEBUG			0x8000

#define setCurlF( _h, F )			_h->flags |= CURLW_FLAG_##F
#define cutCurlF( _h, F )			_h->flags &= ~CURLW_FLAG_##F
#define chkCurlF( _h, F )			( _h->flags & CURLW_FLAG_##F )

#define setCurlFl( _h, F )			_h.flags |= CURLW_FLAG_##F
#define cutCurlFl( _h, F )			_h.flags &= ~CURLW_FLAG_##F
#define chkCurlFl( _h, F )			( _h.flags & CURLW_FLAG_##F )

#define DEFAULT_CURLW_BUFFER		0x10000		// 64k
#define JSON_CONTENT_TYPE			"application/json"
#define JSON_CONTENT_TYPE_LEN		16


struct curlw_times
{
	double							dns;
	double							connect;
	double							appconn;
	double							pretrans;
	double							firstbyte;
	double							total;
	double							redirect;
};

struct curlw_handle
{
	const char					*	url;
	char						*	proxy;
	FILE						*	fh;			// may be allocated by curl_to_buffer
	struct curl_slist			*	hdrs;
	CURLWT						*	times;		// timings for the fetch
	IOBUF						*	iobuf;		// may be allocated by curl_to_buffer
	int64_t							size;		// filled in after fetch
	int								flags;		// fetch flags
	int								error;		// setup error
	curlw_cb					*	cb;			// callback for full iobuf
	curlw_jcb					*	jcb;		// callback for json object
	void						*	arg;		// argument for callback
	JSON						*	jso;		// parsed json object
};


// fetch to iobuf or file; send data to POST
int curlw_fetch( CURLWH *ch, char *data, size_t len );
CURLWH *curlw_handle( char *url, int flags, curlw_cb *cb, curlw_jcb *jcb, void *arg, IOBUF *buf );


#endif
