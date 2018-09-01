/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* curlw.c - wraps around curl functions                                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "shared.h"

// curl options check
// can't use verify below 7.41.0
#if LIBCURL_VERSION_MAJOR > 7 || \
  ( LIBCURL_VERSION_MAJOR == 7 && \
    LIBCURL_VERSION_MINOR > 40 )
	#define _LCURL_CAN_VERIFY 1
#else
	#define _LCURL_CAN_VERIFY 0
#endif


// set up curl, and do appropriate checks
int curlw_setup( CURL **cp, char *url, int flags )
{
	CURL *c;

	*cp = NULL;

	// set up our new context
	if( !( c = curl_easy_init( ) ) )
	{
		err( "Could not init curl for url fetch -- %s", Err );
		return -1;
	}

	curl_easy_setopt( c, CURLOPT_URL, url );
	*cp = c;

	if( chkCurlF( flags, SLOW ) )
		curl_easy_setopt( c, CURLOPT_TIMEOUT_MS, 120000 );
	else
		curl_easy_setopt( c, CURLOPT_TIMEOUT_MS, 10000 );

	curl_easy_setopt( c, CURLOPT_CONNECTTIMEOUT_MS, 5000 );

#if _LCURL_CAN_VERIFY > 0
	if( chkCurlF( flags, SSL ) )
	{
		if( chkCurlF( flags, VALIDATE ) )
		{
			curl_easy_setopt( c, CURLOPT_SSL_VERIFYPEER,   1L );
			curl_easy_setopt( c, CURLOPT_SSL_VERIFYHOST,   1L );
			curl_easy_setopt( c, CURLOPT_SSL_VERIFYSTATUS, 1L );
		}
		else
		{
			curl_easy_setopt( c, CURLOPT_SSL_VERIFYPEER,   0L );
			curl_easy_setopt( c, CURLOPT_SSL_VERIFYHOST,   0L );
			curl_easy_setopt( c, CURLOPT_SSL_VERIFYSTATUS, 0L );
		}
	}
#endif

	return 0;
}





// curl to file, deleting it before writing

FILE *curlw_to_file( char *url, int flags )
{
	FILE *fh = NULL;
	CURLcode cc;
	CURL *c;

	if( curlw_setup( &c, url, flags ) != 0 )
		goto CURLW_FILE_CLEANUP;

	if( !( fh = tmpfile( ) ) )
	{
		err( "Could not open temporary file -- %s", Err );
		goto CURLW_FILE_CLEANUP;
	}

	// and set the file handle
	curl_easy_setopt( c, CURLOPT_WRITEDATA, (void *) fh );

	// OK, go get it
	if( ( cc = curl_easy_perform( c ) ) != CURLE_OK )
	{
		err( "Could not curl target url '%s' -- %s", url, CErr );
		goto CURLW_FILE_CLEANUP;
	}

	// and rewind
	fseek( fh, 0L, SEEK_SET );

CURLW_FILE_CLEANUP:
	curl_easy_cleanup( c );
	return fh;
}


static size_t curlw_write_iobuf( void *contents, size_t size, size_t nmemb, void *userp )
{
	IOBUF *b = (IOBUF *) userp;
	int32_t csz;

	csz = (int32_t) ( size * nmemb );

	// will this fit?
	if( ( csz + b->len ) > b->sz )
	{
		b->sz  = b->len + csz + 1;
		b->ptr = realloc( b->ptr, b->sz );

		if( !b->ptr )
		{
			err( "Could not allocate buffer memory to size %d.", b->sz );
			return 0;
		}

		b->buf  = b->ptr;
		b->hwmk = ( 5 * b->sz ) / 6;
	}

	memcpy( b->buf + b->len, contents, csz );
	b->len += csz;
	b->buf[b->len] = '\0';

	return csz;
}


int curlw_to_buffer( char *url, int flags, IOBUF *b )
{
	int ret = -1;
	CURLcode cc;
	CURL *c;

	if( curlw_setup( &c, url, flags ) != 0 )
		goto CURLW_BUF_CLEANUP;

	curl_easy_setopt( c, CURLOPT_WRITEDATA, (void *) b );
	curl_easy_setopt( c, CURLOPT_WRITEFUNCTION, curlw_write_iobuf );

	if( ( cc = curl_easy_perform( c ) ) != CURLE_OK )
	{
		err( "Could not curl target url '%s' -- %s", url, CErr );
		goto CURLW_BUF_CLEANUP;
	}

	// it worked!
	ret = 0;

CURLW_BUF_CLEANUP:
	curl_easy_cleanup( c );
	return ret;
}



