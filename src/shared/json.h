/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* json.h - macros to hand-crank json output                               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_JSON_H
#define SHARED_JSON_H


/*
 * These macros help with writing to the BUF object of an HTREQ.
 * They assume it is present in the function, and called 'b'.
 */


#define json_str( _s )			strbuf_aprintf( b, "%s", _s )
#define json_prune( )			if( strbuf_lastchar( b ) == ',' ) { strbuf_chop( b ); }

#define json_starta( )			strbuf_aprintf( b, "[" )
#define json_starto( )			strbuf_aprintf( b, "{" )
#define json_fldo( _f )			strbuf_aprintf( b, "\"%s\": {", _f )
#define json_flda( _f )			strbuf_aprintf( b, "\"%s\": [", _f )
#define json_endo( )			json_prune( ); strbuf_aprintf( b, "}," )
#define json_enda( )			json_prune( ); strbuf_aprintf( b, "]," )
#define json_finisho( )			json_prune( ); strbuf_aprintf( b, "}\n" )
#define json_finisha( )			json_prune( ); strbuf_aprintf( b, "]\n" )

#define json_flds( _f, _v )		strbuf_aprintf( b, "\"%s\": \"%s\",", _f, _v )
#define json_fldi( _f, _v )		strbuf_aprintf( b, "\"%s\": %d,", _f, _v )
#define json_fldu( _f, _v )		strbuf_aprintf( b, "\"%s\": %u,", _f, _v )
#define json_fldI( _f, _v )		strbuf_aprintf( b, "\"%s\": %ld,", _f, _v )
#define json_fldU( _f, _v )		strbuf_aprintf( b, "\"%s\": %lu,", _f, _v )
#define json_fldf( _f, _v )		strbuf_aprintf( b, "\"%s\": %f,", _f, _v )
#define json_fldf6( _f, _v )	strbuf_aprintf( b, "\"%s\": %.6f,", _f, _v )
#define json_fldh( _f, _v )		strbuf_aprintf( b, "\"%s\": %hd,", _f, _v )
#define json_fldH( _f, _v )		strbuf_aprintf( b, "\"%s\": %hu,", _f, _v )




#endif

