/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
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

#define setCurlF( _h, F )			_h->flags |= CURLW_FLAG_##F
#define cutCurlF( _h, F )			_h->flags &= ~CURLW_FLAG_##F
#define chkCurlF( _h, F )			( _h->flags & CURLW_FLAG_##F )

#define setCurlFl( _h, F )			_h.flags |= CURLW_FLAG_##F
#define cutCurlFl( _h, F )			_h.flags &= ~CURLW_FLAG_##F
#define chkCurlFl( _h, F )			( _h.flags & CURLW_FLAG_##F )

#define DEFAULT_CURLW_BUFFER		0x10000		// 64k
#define JSON_CONTENT_TYPE			"application/json"
#define JSON_CONTENT_TYPE_LEN		16


struct curlw_container
{
	CURL						*	handle;
	struct curl_slist			*	hdrs;
};

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
	char						*	url;
	FILE						*	fh;			// may be allocated by curl_to_buffer
	CURLWC						*	ctr;		// container for libcurl objects
	CURLWT						*	times;		// timings for the fetch
	IOBUF						*	iobuf;		// may be allocated by curl_to_buffer
	int64_t							size;		// filled in after fetch
	int								flags;		// fetch flags
	curlw_cb					*	cb;			// callback for full iobuf
	curlw_jcb					*	jcb;		// callback for json object
	void						*	arg;		// argument for callback
	json_object					*	jso;		// parsed json object
};


// fetch to iobuf or file
int curlw_fetch( CURLWH *ch );


#endif
