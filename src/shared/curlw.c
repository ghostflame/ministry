/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* curlw.c - wraps around curl functions                                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "shared.h"


// curl to file, deleting it before writing

FILE *curlw_to_file( char *url, int flags )
{
	FILE *fh = NULL;
	CURLcode cc;
	CURL *c;

	// set up our new context
	if( !( c = curl_easy_init( ) ) )
	{
		err( "Could not init curl for url fetch -- %s", Err );
		return NULL;
	}

	curl_easy_setopt( c, CURLOPT_URL, url );

	if( curlf_chk( flags, SLOW ) )
		curl_easy_setopt( c, CURLOPT_TIMEOUT_MS, 120000 );
	else
		curl_easy_setopt( c, CURLOPT_TIMEOUT_MS, 10000 );

	curl_easy_setopt( c, CURLOPT_CONNECTTIMEOUT_MS, 5000 );

#if _LCURL_CAN_VERIFY > 0
	if( curlf_chk( flags, SSL ) )
	{
		if( curlf_chk( flags, VALIDATE ) )
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

	if( !( fh = tmpfile( ) ) )
	{
		err( "Could not open temporary file -- %s", Err );
		goto CURLW_FETCH_CLEANUP;
	}

	// and set the file handle
	curl_easy_setopt( c, CURLOPT_WRITEDATA, (void *) fh );

	// OK, go get it
	if( ( cc = curl_easy_perform( c ) ) != CURLE_OK )
	{
		err( "Could not curl target url '%s' -- %s", url, CErr );
		goto CURLW_FETCH_CLEANUP;
	}

	// and rewind
	fseek( fh, 0L, SEEK_SET );

CURLW_FETCH_CLEANUP:
	curl_easy_cleanup( c );
	return fh;
}



