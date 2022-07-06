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
* curlw.c - wraps around curl functions                                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "shared.h"

char *ctype_names[CURLINFO_END] =
{
	"text",
	"header-in",
	"header-out",
	"data-in",
	"data-out",
	"ssl-data-in",
	"ssl-data-out"
};

int curlw_debug( CURL *c, curl_infotype type, char *data, size_t sz, void *ptr )
{
	debug( "[CURLDEBUG]: %s (%lu): %s", ctype_names[type], sz, data );
	return 0;
}

#define curlsetopt( _OPT, arg )			if( curl_easy_setopt( c, CURLOPT_##_OPT, arg ) != CURLE_OK ) warn( "Curl setopt failed for %s", #_OPT )


// set up curl, and do appropriate checks
void curlw_setup( CURLWH *ch, CURL *c, char *data, size_t len )
{
	char hdrbuf[8192];

	if( !ch->times )
		ch->times = (CURLWT *) allocz( sizeof( CURLWT ) );

	curlsetopt( URL, ch->url );

	if( ch->proxy && *(ch->proxy) )
	{
		curlsetopt( PROXY, ch->proxy );
	}

	if( chkCurlF( ch, SLOW ) )
	{
		curlsetopt( TIMEOUT_MS, 120000 );
	}
	else
	{
		curlsetopt( TIMEOUT_MS, 10000 );
	}

	if( chkCurlF( ch, DEBUG ) )
	{
		curlsetopt( DEBUGDATA, ch );
		curlsetopt( DEBUGFUNCTION, curlw_debug );
	}

	if( len )
	{
		// say we are going to POST data, and cue it up
		curlsetopt( POST, 1L );
		curlsetopt( POSTFIELDS, data );
		curlsetopt( POSTFIELDSIZE, len );
	}

	curlsetopt( CONNECTTIMEOUT_MS, 5000 );

	if( chkCurlF( ch, SSL ) && runf_has( RUN_CURL_VERIFY ) )
	{
		// these return success/failure, but we cannot check
		// them meaningfully because older versions of libcurl
		// (looking at you, CentOS/RHEL 7) do not do all of
		// these checks, so we just do our best.  If you want
		// cert verification, use an OS with up to date curl.
		if( chkCurlF( ch, VALIDATE ) )
		{
			curlsetopt( SSL_VERIFYPEER,   1L );
			curlsetopt( SSL_VERIFYHOST,   2L );
			curlsetopt( SSL_VERIFYSTATUS, 1L );
		}
		else
		{
			curlsetopt( SSL_VERIFYPEER,   0L );
			curlsetopt( SSL_VERIFYHOST,   0L );
			curlsetopt( SSL_VERIFYSTATUS, 0L );
		}
	}

	ch->hdrs = NULL;

	// if we are expecting to parse the json we get back,
	// say that we accept json
	if( chkCurlF( ch, PARSE_JSON ) )
	{
		snprintf( hdrbuf, 8192, "%s: %s", ACCEPT_HDR, JSON_CONTENT_TYPE );
		ch->hdrs = curl_slist_append( ch->hdrs, hdrbuf );
	}

	// are we posting json?
	if( len && chkCurlF( ch, SEND_JSON ) )
	{
		snprintf( hdrbuf, 8192, "%s: %s", CONTENT_TYPE_HDR, JSON_CONTENT_TYPE );
		ch->hdrs = curl_slist_append( ch->hdrs, hdrbuf );
	}

	// got any headers?
	if( ch->hdrs )
	{
		curlsetopt( HTTPHEADER, ch->hdrs );
	}
}




#define cegit( lbl, tgt )		curl_easy_getinfo( c, CURLINFO_##lbl##_TIME, &(ch->times->tgt) )

void curlw_get_times( CURLWH *ch, CURL *c )
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


size_t curlw_write_buf( void *contents, size_t size, size_t nmemb, void *userp )
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
				err( "Could not allocate buffer memory to size %d.", ch->iobuf->bf->sz );

			return 0;
		}
		b = ch->iobuf;
	}

	cp = (uint8_t *) contents;
	rem = csz;

	// copy to buffer and call the callback when it gets full
	while( rem > 0 )
	{
		max = strbuf_space( b->bf );
		len = ( rem > max ) ? max : rem;

		buf_appends( b->bf, cp, len );
		buf_terminate( b->bf );

		rem -= len;
		cp  += len;

		if( b->bf->len > b->hwmk && ch->cb )
			(*(ch->cb))( ch->arg, b );
	}

	return csz;
}




void curlw_file_to_cb( CURLWH *ch )
{
	off_t sz, rb;
	BUF *b;
	int d;

	d = fileno( ch->fh );
	b = ch->iobuf->bf;

	// how big?
	sz = lseek( d, 0L, SEEK_END );
	lseek( d, 0L, SEEK_SET );

	strbuf_empty( b );

	while( sz > 0 )
	{
		rb = strbuf_space( b );
		if( rb > sz )
			rb = sz;

		// read it in, continuing lines if needed
		if( !fread( b->buf + b->len, rb, 1, ch->fh ) )
		{
			warn( "Failed to read in tmpfile from fetch -- %s", Err );
			break;
		}
		b->len += rb;

		// and call the callback on the buffer
		(*(ch->cb))( ch->arg, ch->iobuf );

		sz -= rb;
	}
}




// try to curl to buffer, but fall back to file if the
// response is bigger than we say we're allowed
int curlw_fetch( CURLWH *ch, char *data, size_t len )
{
	CURL *c = NULL;
	int ret = -1;
	CURLcode cc;
	char *cty;

	if( !data || !*data )
		len = 0;

	// set up our new context
	if( !( c = curl_easy_init( ) ) )
	{
		if( chkCurlF( ch, VERBOSE ) )
			err( "Could not init curl for url fetch -- %s", Err );

		return -1;
	}

	ch->size = 0;
	curlw_setup( ch, c, data, len );

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

		curlsetopt( WRITEDATA, (void *) ch->fh );
	}
	else
	{
		curlsetopt( WRITEFUNCTION, &curlw_write_buf );
		curlsetopt( WRITEDATA, (void *) ch );
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
	 && ( curl_easy_getinfo( c, CURLINFO_CONTENT_TYPE, &cty ) != CURLE_OK
	   || !cty
	   || strncasecmp( cty, JSON_CONTENT_TYPE, JSON_CONTENT_TYPE_LEN ) ) )
		cutCurlF( ch, PARSE_JSON );

	// do we want timings?
	if( chkCurlF( ch, TIMINGS ) )
		curlw_get_times( ch, c );

CURLW_FETCH_CLEANUP:
	if( ch->hdrs )
	{
		curl_slist_free_all( ch->hdrs );
		ch->hdrs = NULL;
	}

	curl_easy_cleanup( c );

	// rewind that filehandle?  or close it
	if( chkCurlF( ch, TOFILE ) && ch->fh )
	{
		if( ret == 0 )
		{
			fseek( ch->fh, 0L, SEEK_SET );

			if( chkCurlF( ch, PARSE_JSON ) )
			{
				ch->jso = parse_json_file( ch->fh, NULL );
				if( ch->jso )
				{
					if( ch->jcb )
						(*(ch->jcb))( ch->arg, ch->jso );

					// and delete that
					json_object_put( ch->jso );
					ch->jso = NULL;
				}
				else
				{
					warn( "Could not parse json received from '%s'", ch->url );
					fseek( ch->fh, 0L, SEEK_SET );
					ret = 1;
				}
			}
			else
			{
				// read it back in and hand it to the callback
				curlw_file_to_cb( ch );
			}
		}

		fclose( ch->fh );
		ch->fh = NULL;
	}
	// or call the buffer callback one last time
	else if( ch->iobuf && ch->iobuf->bf->len > 0 && ch->cb )
		(*(ch->cb))( ch->arg, ch->iobuf );

	return ret;
}



CURLWH *curlw_handle( char *url, int flags, curlw_cb *cb, curlw_jcb *jcb, void *arg, IOBUF *buf )
{
	CURLWH *ch;

	ch = (CURLWH *) allocz( sizeof( CURLWH ) );

	ch->flags = flags;
	ch->cb    = cb;
	ch->jcb   = jcb;
	ch->arg   = arg;

	if( url )
		ch->url = str_copy( url, 0 );

	if( buf )
		ch->iobuf = buf;
	else
		ch->iobuf = mem_new_iobuf( NET_BUF_SZ );

	return ch;
}
