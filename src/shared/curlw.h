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


#define CErr						curl_easy_strerror( cc )

#define CURLW_FLAG_SSL				0x01
#define CURLW_FLAG_VALIDATE			0x02
#define CURLW_FLAG_SLOW				0x10
#define CURLW_FLAG_TOFILE			0x20

#define setCurlF( _h, F )			_h->flags |= CURLW_FLAG_##F
#define cutCurlF( _h, F )			_h->flags &= ~CURLW_FLAG_##F
#define chkCurlF( _h, F )			( _h->flags & CURLW_FLAG_##F )

#define setCurlFl( _h, F )			_h.flags |= CURLW_FLAG_##F
#define cutCurlFl( _h, F )			_h.flags &= ~CURLW_FLAG_##F
#define chkCurlFl( _h, F )			( _h.flags & CURLW_FLAG_##F )

#define DEFAULT_CURLW_BUFFER		0x10000		// 64k

struct curlw_handle
{
	char						*	url;
	FILE						*	fh;			// may be allocated by curl_to_buffer
	IOBUF						*	iobuf;		// may be allocated by curl_to_buffer
	int64_t							size;		// filled in after fetch
	int								flags;		// fetch flags
	curlw_cb					*	cb;			// callback for full iobuf
	void						*	arg;		// argument for callback
};


// fetch to iobuf or file
int curlw_fetch( CURLWH *ch );


#endif
