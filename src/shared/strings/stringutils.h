/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
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
	char				*	space;
	char				*	buf;
	uint32_t				len;
	uint32_t				sz;
};



struct string_control
{
	SSTR				*	stores;

	// permanent string space
	BUF					*	perm;

	// permanent string locking
	pthread_mutex_t			perm_lock;
	// string store locking
	pthread_mutex_t			store_lock;
};



// FUNCTIONS


// allocation of strings that can't be freed
char *str_perm( uint32_t len );
char *str_dup( char *src, int len );

// this can be freed
char *str_copy( char *src, int len );

// get a buffer
BUF *strbuf( uint32_t size );
BUF *strbuf_create( char *str, int len );
void strbuf_free( BUF *b );

// modify a buffer
BUF *strbuf_resize( BUF *b, uint32_t size );
BUF *strbuf_copy( BUF *b, char *str, int len );
int strbuf_copymax( BUF *b, char *str, int len );
BUF *strbuf_add( BUF *b, char *str, int len );
void strbuf_keep( BUF *b, int len );		// keep the last len bytes, move to the start

// write json to a buffer
BUF *strbuf_json( BUF *b, json_object *o, int done );

// these work as macros
#define strbuf_printf( b, ... )			b->len = snprintf( b->buf, b->sz, ##__VA_ARGS__ )
#define strbuf_vprintf( b, ... )		b->len = vsnprintf( b->buf, b->sz, ##__VA_ARGS__ )
#define strbuf_aprintf( b, ... )		b->len += snprintf( b->buf + b->len, b->sz - b->len, ##__VA_ARGS__ )
#define strbuf_avprintf( b, ... )		b->len += vsnprintf( b->buf + b->len, b->sz - b->len, ##__VA_ARGS__ )
#define strbuf_empty( b )				b->len = 0; b->buf[0] = '\0'
#define strbuf_chop( b )				if( b->len > 0 ) { b->len--; b->buf[b->len] = '\0'; }
#define strbuf_chopn( b, n )			if( b->len > n ) { b->len -= n; b->buf[b->len] = '\0'; } else { strbuf_empty( b ); }
#define strbuf_trunc( b, l )			if( l > b->len ) { b->buf[l] = '\0'; b->len = l; }
#define strbuf_lastchar( b )			( ( b->len ) ? b->buf[b->len - 1] : '\0' )
#define strbuf_append( b, o )			strbuf_copy( strbuf_resize( b, b->len + o->len ), o->buf, o->len )
#define strbuf_space( b )				( b->sz - ( b->len + 1 ) )

// unchecked macros, for callers who have done their homework
#define buf_append( _b, _c )			memcpy( _b->buf + _b->len, _c->buf, _c->len ); _b->len += _c->len
#define buf_appends( _b, _s, _l )		memcpy( _b->buf + _b->len, _s, _l ); _b->len += _l
#define buf_addchar( _b, _c )			_b->buf[_b->len++] = _c
#define buf_terminate( _b )				_b->buf[_b->len] = '\0'
#define buf_hasspace( _b, _l )			( ( _b->len + _l ) < _b->sz )
#define buf_space( _b )					strbuf_space( _b )


// get string length, up to a maximum
int str_nlen( char *src, int max );

// remove NL\CR and report len change
int chomp( char *s, int len );

// remove surrounding whitespace
int trim( char **str, int *len );

// substitute args into strings, using %\d%
int str_sub( char **ptr, int *len, int argc, char **argv, int *argl );

// break up potentially quoted string by delimiter
int strqwords( WORDS *w, char *src, int len, char sep );

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
SSTR *string_store_create( int64_t sz, char *size, int *default_value, int freeable );

// add a string.  Adding stuff onto them is your problem
SSTE *string_store_add( SSTR *store, char *str, int len );

// search for a string - setting val_set means a 0 value in the
// val position is considered INVALID, and skipped over
SSTE *string_store_look( SSTR *store, char *str, int len, int val_set );

// lock or unlock
int string_store_locking( SSTR *store, int lk );

// call a callback on every entry
int string_store_iterator( SSTR *store, void *arg, store_callback *fp );

// clean up those mutexes
void string_store_cleanup( SSTR *list );

// how many?
int64_t string_store_entries( SSTR *store );

void string_deinit( void );
STR_CTL *string_config_defaults( void );
//conf_line_fn string_config_line;

#endif
