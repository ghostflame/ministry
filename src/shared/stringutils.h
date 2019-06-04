/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* strings.h - structures and declarations for string handling             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef SHARED_STRINGUTILS_H
#define SHARED_STRINGUTILS_H


// for fast string allocation, free not possible
#define PERM_SPACE_BLOCK	0x1000000	// 1M

// for breaking up strings on a delimiter
#define STRWORDS_MAX		339			// makes for a 4k struct


struct words_data
{
	char				*	wd[STRWORDS_MAX];
	char				*	end;
	int						len[STRWORDS_MAX];
	int						end_len;
	int						in_len;
	int						wc;
};

struct string_store_entry
{
	SSTE				*	next;
	char				*	str;
	int32_t					len;
	int32_t					val;	// if you want to store something here
	void				*	ptr;	// if you want to store something here
};


struct string_store
{
	SSTR				*	next;		// in case the caller wants a list
	SSTR				*	_proc_next;	// a list to live in _proc
	SSTE				**	hashtable;

	int64_t					hsz;
	int64_t					entries;
	int32_t					val_default;
	int32_t					set_default;

	pthread_mutex_t			mtx;
};


struct string_buffer
{
	char				*	space;
	char				*	buf;
	uint32_t				len;
	uint32_t				sz;
};


// FUNCTIONS


// allocation of strings that can't be freed
char *perm_str( int len );
char *str_dup( char *src, int len );

#define dup_val( )						str_dup( av->vptr, av->vlen )

// this can be freed
char *str_copy( char *src, int len );

// get a buffer
BUF *strbuf( uint32_t size );
BUF *strbuf_create( char *str, int len );

// modify a buffer
BUF *strbuf_resize( BUF *b, uint32_t size );
BUF *strbuf_copy( BUF *b, char *str, int len );
BUF *strbuf_add( BUF *b, char *str, int len );

// these work as macros
#define strbuf_printf( b, ... )			b->len = snprintf( b->buf, b->sz, ##__VA_ARGS__ )
#define strbuf_aprintf( b, ... )		b->len += snprintf( b->buf + b->len, b->sz - b->len, ##__VA_ARGS__ )
#define strbuf_empty( b )				b->len = 0; b->buf[0] = '\0'
#define strbuf_chop( b )				if( b->len > 0 ) { b->len--; b->buf[b->len] = '\0'; }
#define strbuf_chopn( b, n )			if( b->len > n ) { b->len -= n; b->buf[b->len] = '\0'; } else { strbuf_empty( b ); }
#define strbuf_lastchar( b )			( ( b->len ) ? b->buf[b->len - 1] : '\0' )
#define strbuf_append( b, o )			strbuf_copy( strbuf_resize( b, b->len + o->len ), o->buf, o->len )


// get string length, up to a maximum
int str_nlen( char *src, int max );

// substitute args into strings, using %\d%
int strsub( char **ptr, int *len, int argc, char **argv, int *argl );

// break up potentially quoted string by delimiter
int strqwords( WORDS *w, char *src, int len, char sep );

// break up a string by delimiter*
int strwords_multi( WORDS *w, char *src, int len, char sep, int8_t multi );

// with and without multi separate behaviour
#define strwords( _w, _s, _l, _c )		strwords_multi( _w, _s, _l, _c, 0 )
#define strmwords( _w, _s, _l, _c )		strwords_multi( _w, _s, _l, _c, 1 )




// string store - store strings as keys with optional values
// they cannot be removed, but you can use the val as a removal bit

// pass either an int or a char size.  Default is "medium"
SSTR *string_store_create( int64_t sz, char *size, int *default_value );

// add a string.  Adding stuff onto them is your problem
SSTE *string_store_add( SSTR *store, char *str, int len );

// search for a string - setting val_set means a 0 value in the
// val position is considered INVALID, and skipped over
SSTE *string_store_look( SSTR *store, char *str, int len, int val_set );

// lock or unlock
int string_store_locking( SSTR *store, int lk );

// clean up those mutexes
void string_store_cleanup( SSTR *list );



#endif
