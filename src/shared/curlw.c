/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* curlw.c - wraps around curl functions                                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "shared.h"


// set up curl, and do appropriate checks
int curlw_setup( CURL **cp, CURLWH *ch )
{
	CURL *c;

	*cp = NULL;

	// set up our new context
	if( !( c = curl_easy_init( ) ) )
	{
		err( "Could not init curl for url fetch -- %s", Err );
		return -1;
	}

	curl_easy_setopt( c, CURLOPT_URL, ch->url );
	*cp = c;

	if( chkCurlF( ch, SLOW ) )
		curl_easy_setopt( c, CURLOPT_TIMEOUT_MS, 120000 );
	else
		curl_easy_setopt( c, CURLOPT_TIMEOUT_MS, 10000 );

	curl_easy_setopt( c, CURLOPT_CONNECTTIMEOUT_MS, 5000 );

	if( chkCurlF( ch, SSL ) && runf_has( RUN_CURL_VERIFY ) )
	{
		// these return success/failure, but we cannot check
		// them meaningfully because older versions of libcurl
		// (looking at you, CentOS/RHEL 7) do not do all of
		// these checks, so we just do our best.  If you want
		// cert verification, use an OS with up to date curl.
		if( chkCurlF( ch, VALIDATE ) )
		{
			curl_easy_setopt( c, CURLOPT_SSL_VERIFYPEER,   1L );
			curl_easy_setopt( c, CURLOPT_SSL_VERIFYHOST,   2L );
			curl_easy_setopt( c, CURLOPT_SSL_VERIFYSTATUS, 1L );
		}
		else
		{
			curl_easy_setopt( c, CURLOPT_SSL_VERIFYPEER,   0L );
			curl_easy_setopt( c, CURLOPT_SSL_VERIFYHOST,   0L );
			curl_easy_setopt( c, CURLOPT_SSL_VERIFYSTATUS, 0L );
		}
	}

	return 0;
}


static size_t curlw_write_buf( void *contents, size_t size, size_t nmemb, void *userp )
{
	CURLWH *ch = (CURLWH *) userp;
	int32_t csz, rem, len, max;
	uint8_t *cp;
	IOBUF *b;

	csz = (int32_t) ( size * nmemb );
	ch->size += csz;

	b = ch->iobuf;

	// need a new buffer?
	if( !b )
	{
		if( !( ch->iobuf = mem_new_iobuf( DEFAULT_CURLW_BUFFER ) ) )
		{
			err( "Could not allocate buffer memory to size %d.", ch->iobuf->sz );
			return 0;
		}
		b = ch->iobuf;
	}

	cp = (uint8_t *) contents;
	rem = csz;

	// copy to buffer and call the callback when it gets full
	while( rem > 0 )
	{
		max = b->sz - ( b->len + 1 );
		len = ( rem > max ) ? max : rem;

		memcpy( b->buf + b->len, cp, len );
		b->len += len;
		b->buf[b->len] = '\0';

		rem -= len;
		cp  += len;

		if( b->len > b->hwmk && ch->cb )
			(*(ch->cb))( ch->arg, b );
	}

	return csz;
}



// try to curl to buffer, but fall back to file if the
// response is bigger than we say we're allowed
int curlw_fetch( CURLWH *ch )
{
	int ret = -1;
	CURLcode cc;
	CURL *c;

	if( curlw_setup( &c, ch ) != 0 )
		goto CURLW_FETCH_CLEANUP;

	ch->size = 0;

	if( chkCurlF( ch, TOFILE ) )
	{
		if( !( ch->fh = tmpfile( ) ) )
		{
			err( "Could not create tmpfile for curlw results." );
			goto CURLW_FETCH_CLEANUP;
		}

		curl_easy_setopt( c, CURLOPT_WRITEDATA, (void *) ch->fh );
	}
	else
	{
		curl_easy_setopt( c, CURLOPT_WRITEDATA, (void *) ch );
		curl_easy_setopt( c, CURLOPT_WRITEFUNCTION, curlw_write_buf );
	}

	if( ( cc = curl_easy_perform( c ) ) != CURLE_OK )
	{
		err( "Could not curl target url '%s' -- %s", ch->url, CErr );
		goto CURLW_FETCH_CLEANUP;
	}

	// it worked!
	ret = 0;


CURLW_FETCH_CLEANUP:
	curl_easy_cleanup( c );

	// rewind that filehandle?  or close it
	if( chkCurlF( ch, TOFILE ) && ch->fh )
	{
		if( ret == 0 )
			fseek( ch->fh, 0L, SEEK_SET );
		else
		{
			fclose( ch->fh );
			ch->fh = NULL;
		}
	}
	// or call the buffer callback one last time
	else if( ch->iobuf && ch->iobuf->len > 0 && ch->cb )
		(*(ch->cb))( ch->arg, ch->iobuf );

	return ret;
}



