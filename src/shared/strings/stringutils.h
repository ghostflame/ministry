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
* strings/stringutils.h - structures and declarations for string handling *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef SHARED_STRINGUTILS_H
#define SHARED_STRINGUTILS_H

// for breaking up strings on a delimiter
#define STRWORDS_MAX		339			// makes for a 4k struct

struct words_data
{
	char				*	wd[STRWORDS_MAX];
	char				*	end;
	int32_t					len[STRWORDS_MAX];
	int32_t					end_len;
	int32_t					in_len;
	int16_t					wc;
	int8_t					end_on_sep;
	char					sep;
};

#define STRSTORE_FLAG_FREEABLE			0x0001
#define STRSTORE_FLAG_VALID				0x0002


struct string_store_entry
{
	SSTE				*	next;
	char				*	str;
	uint64_t				hv;
	uint16_t				len;
	uint16_t				flags;
	int32_t					val;	// if you want to store something here
	void				*	ptr;	// if you want to store something here
};


struct string_buffer
{
	char				*	buf;
	uint32_t				len;
	uint32_t				sz;
};


struct string_control
{
	SSTR				*	stores;

	// string store locking
	pthread_mutex_t			store_lock;
};



// FUNCTIONS


// allocation of strings that can't be freed
char *str_perm( const char *src, int len );

// this can be freed
char *str_copy( const char *src, int len );

// get a buffer
BUF *strbuf( uint32_t size );
BUF *strbuf_create( const char *str, int len );
void strbuf_free( BUF *b );

// modify a buffer
BUF *strbuf_resize( BUF *b, uint32_t size );
BUF *strbuf_copy( BUF *b, const char *str, int len );
int strbuf_copymax( BUF *b, const char *str, int len );
BUF *strbuf_add( BUF *b, const char *str, int len );
void strbuf_keep( BUF *b, int len );		// keep the last len bytes, move to the start

// write json to a buffer
BUF *strbuf_json( BUF *b, json_object *o, int done );

// unchecked macros, for callers who have done their homework
#define buf_append( _b, _c )			memcpy( _b->buf + _b->len, _c->buf, _c->len ); _b->len += _c->len
#define buf_appends( _b, _s, _l )		memcpy( _b->buf + _b->len, _s, _l ); _b->len += _l
#define buf_addchar( _b, _c )			_b->buf[_b->len++] = _c
#define buf_terminate( _b )				_b->buf[_b->len] = '\0'
#define buf_hasspace( _b, _l )			( ( _b->len + _l ) < _b->sz )
#define buf_space( _b )					strbuf_space( _b )
#define buf_tostring( _b, _s )			memcpy( _s, _b->buf, _b->len )


// these work as macros - but DO NOT EXPAND b->sz
#define strbuf_printf( _b, ... )		_b->len  =  snprintf( _b->buf, _b->sz, ##__VA_ARGS__ )
#define strbuf_vprintf( _b, ... )		_b->len  = vsnprintf( _b->buf, _b->sz, ##__VA_ARGS__ )
#define strbuf_aprintf( _b, ... )		_b->len +=  snprintf( _b->buf + _b->len, _b->sz - _b->len, ##__VA_ARGS__ )
#define strbuf_avprintf( _b, ... )		_b->len += vsnprintf( _b->buf + _b->len, _b->sz - _b->len, ##__VA_ARGS__ )

#define strbuf_empty( _b )				    _b->len = 0;                    buf_terminate( _b )
#define strbuf_chopn( _b, _n )			if( _b->len > _n ) { _b->len -= _n; buf_terminate( _b ); } else { strbuf_empty( _b ); }
#define strbuf_trunc( _b, _l )			if( _b->len > _l ) { _b->len = _l;  buf_terminate( _b ); }
#define strbuf_chop( _b )				strbuf_chopn( _b, 1 )

#define strbuf_lastchar( _b )			( ( _b->len ) ? _b->buf[_b->len - 1] : '\0' )
#define strbuf_append( _b, _o )			strbuf_copy( _b, _o->buf, _o->len )
#define strbuf_space( _b )				( _b->sz - ( _b->len + 1 ) )



// get string length, up to a maximum
int str_nlen( const char *src, int max );

// remove NL\CR and report len change
int chomp( char *s, int len );

// remove surrounding whitespace
int trim( char **str, int *len );

// substitute args into strings, using %\d%
int str_sub( char **ptr, int *len, int argc, char **argv, int *argl );

// find a string in a list
int str_search( const char *str, const char **list, int len );

// break up potentially quoted string by delimiter
int strqwords( WORDS *w, char *src, int len, char sep );

// see if strings begin the same - useful for short matches
// eg "mil" matches "milli" matches "millimeters"
int strprefix( const char *a, const char *b );

// break up a string by delimiter*
int strwords_multi( WORDS *w, char *src, int len, char sep, int8_t multi );

// recreate a string
int strwords_join( WORDS *w, char *filler, char **dst, int *sz );

// put the keys of a json object into a words structure
int strwords_json_keys( WORDS *w, JSON *jo );

// with and without multi separate behaviour
#define strwords( _w, _s, _l, _c )		strwords_multi( _w, _s, _l, _c, 0 )
#define strmwords( _w, _s, _l, _c )		strwords_multi( _w, _s, _l, _c, 1 )
#define strlines( _w, _s, _l )			strwords_multi( _w, _s, _l, '\n', 1 )


#define BOOL_ENABLED( _b )				( _b ) ? "en" : "dis"
#define VAL_PLURAL( _v )				( _v == 1 ) ? "" : "s"


// string store - store strings as keys with optional values
// they cannot be removed, but you can use the val as a removal bit

// pass either an int or a char size.  Default is "medium"
SSTR *string_store_create( int64_t sz, const char *size, int *default_value, int freeable );

// add an entry, with appropriate values and such
SSTE *string_store_add_with_vals( SSTR *store, const char *str, int len, int32_t *val, void *ptr );

// add a string.  Adding stuff onto them is your problem
#define string_store_add( _st, _s, _l )		string_store_add_with_vals( _st, _s, _l, NULL, NULL )

// search for a string - setting val_set means a 0 value in the
// val position is considered INVALID, and skipped over
SSTE *string_store_look( SSTR *store, const char *str, int len, int val_set );

// lock or unlock
int string_store_locking( SSTR *store, int lk );

// call a callback on every entry
int string_store_iterator( SSTR *store, void *arg, store_callback *fp, int idx, int mod );
int string_store_iterator_nolock( SSTR *store, void *arg, store_callback *fp, int idx, int mod );

// clean up those mutexes
void string_store_cleanup( SSTR *list );

// how many?
int64_t string_store_entries( SSTR *store );

void string_deinit( void );
STR_CTL *string_config_defaults( void );
//conf_line_fn string_config_line;

#endif
