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

#define setCurlF( _i, F )			_i |= CURLW_FLAG_##F
#define curCurlF( _i, F )			_i &= ~CURLW_FLAG_##F
#define chkCurlF( _i, F )			( _i & CURLW_FLAG_##F )





// fetch to file
FILE *curlw_to_file( char *url, int flags );

// fetch to iobuf
int curlw_to_buffer( char *url, int flags, IOBUF *b );


#endif
