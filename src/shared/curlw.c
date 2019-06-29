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
int curlw_setup( CURLWH *ch )
{
	CURL *c;

	if( !ch->ctr )
		ch->ctr = (CURLWC *) allocz( sizeof( CURLWC ) );

	if( !ch->times )
		ch->times = (CURLWT *) allocz( sizeof( CURLWT ) );


	// set up our new context
	if( !( ch->ctr->handle = curl_easy_init( ) ) )
	{
		if( chkCurlF( ch, VERBOSE ) )
			err( "Could not init curl for url fetch -- %s", Err );

		return -1;
	}

	c = ch->ctr->handle;

	curl_easy_setopt( c, CURLOPT_URL, ch->url );

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

	// if we are expecting to parse the json we get back,
	// say that we accept json
	if( chkCurlF( ch, PARSE_JSON ) )
	{
		ch->ctr->hdrs = curl_slist_append( ch->ctr->hdrs, "Accept: " CURLW_JSON_CT );
		curl_easy_setopt( c, CURLOPT_HTTPHEADER, ch->ctr->hdrs );
	}

	return 0;
}


// we are expecting the data to be in a file
int curlw_parse_json( CURLWH *ch )
{
	enum json_tokener_error jerr;
	char *bf;
	off_t sz;
	int fd;

	fd = fileno( ch->fh );
	sz = lseek( fd, 0, SEEK_END );
	bf = mmap( NULL, sz, PROT_READ, MAP_PRIVATE, fd, 0 );

	ch->jso = json_tokener_parse_verbose( bf, &jerr );

	if( jerr != json_tokener_success )
	{
		json_object_put( ch->jso );
		ch->jso = NULL;
		return -1;
	}

	return 0;
}



int curlw_is_json( CURLWH *ch )
{
	CURLcode ret;
	char *ct;
	int n;

	ret = curl_easy_getinfo( ch->ctr->handle, CURLINFO_CONTENT_TYPE, &ct );

	if( ret == CURLE_OK )
	{
		n = strlen( CURLW_JSON_CT );
		if( !strncasecmp( ct, CURLW_JSON_CT, n ) )
			return 1;
	}

	return 0;
}

#define cegit( lbl, tgt )		curl_easy_getinfo( ch->ctr->handle, CURLINFO_##lbl##_TIME, &(ch->times->tgt) )

void curlw_get_times( CURLWH *ch )
{
	memset( ch->times, 0, sizeof( CURLWT ) );

	cegit( NAMELOOKUP, dns );
	cegit( CONNECT, connect );
	cegit( APPCONNECT, appconn );
	cegit( PRETRANSFER, pretrans );
	cegit( STARTTRANSFER, firstbyte );
	cegit( TOTAL, total );
	cegit( REDIRECT, redirect );
}

#undef cegit


static size_t curlw_write_buf( void *contents, size_t size, size_t nmemb, void *userp )
{
	CURLWH *ch = (CURLWH *) userp;
	int32_t csz, rem, len, max;
	uint8_t *cp;
	IOBUF *b;

	csz = (int32_t) ( size * nmemb );
	ch->size += csz;

	// need a new buffer?
	if( !( b = ch->iobuf ) )
	{
		if( !( ch->iobuf = mem_new_iobuf( DEFAULT_CURLW_BUFFER ) ) )
		{
			if( chkCurlF( ch, VERBOSE ) )
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
	CURL *c = NULL;
	int ret = -1;
	CURLcode cc;

	if( curlw_setup( ch ) != 0 )
		goto CURLW_FETCH_CLEANUP;

	c = ch->ctr->handle;
	ch->size = 0;

	// we put json in a file to avoid having
	// to set limits on size
	if( chkCurlF( ch, PARSE_JSON ) )
		setCurlF( ch, TOFILE );

	// make a temp file fo the output?
	if( chkCurlF( ch, TOFILE ) )
	{
		if( !( ch->fh = tmpfile( ) ) )
		{
			if( chkCurlF( ch, VERBOSE ) )
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
		if( chkCurlF( ch, VERBOSE ) )
			err( "Could not curl target url '%s' -- %s", ch->url, CErr );

		goto CURLW_FETCH_CLEANUP;
	}

	// it worked!
	ret = 0;

	// was it json?
	if( chkCurlF( ch, PARSE_JSON )
	 && !curlw_is_json( ch ) )
	{
		warn( "Parse json requested, but response is not json from url '%s'", ch->url );
		cutCurlF( ch, PARSE_JSON );
	}

	if( chkCurlF( ch, TIMINGS ) )
		curlw_get_times( ch );

CURLW_FETCH_CLEANUP:
	curl_easy_cleanup( c );

	if( ch->ctr->hdrs )
		curl_slist_free_all( ch->ctr->hdrs );

	memset( ch->ctr, 0, sizeof( CURLWC ) );

	// rewind that filehandle?  or close it
	if( chkCurlF( ch, TOFILE ) && ch->fh )
	{
		if( ret == 0 )
		{
			fseek( ch->fh, 0L, SEEK_SET );

			if( chkCurlF( ch, PARSE_JSON ) )
			{
				if( curlw_parse_json( ch ) )
				{
					warn( "Could not parse json received from '%s'", ch->url );
					fseek( ch->fh, 0L, SEEK_SET );
					ret = 1;
				}
				else
				{
					if( ch->jcb )
						(*(ch->jcb))( ch->arg, ch->jso );

					// close the file down
					fclose( ch->fh );
					ch->fh = NULL;

					// and delete that
					json_object_put( ch->jso );
					ch->jso = NULL;
				}
			}
		}
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



