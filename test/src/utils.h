/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* utils.h - routine declations and utility structures                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef MINISTRY_TEST_UTILS_H
#define MINISTRY_TEST_UTILS_H


// for fast string allocation, free not possible
#define PERM_SPACE_BLOCK	0x1000000	// 1M

// for breaking up strings on a delimiter
#define STRWORDS_MAX		339			// makes for a 4k struct

enum num_types
{
	NUM_INVALID = -1,
	NUM_NORMAL = 0,
	NUM_OCTAL,
	NUM_HEX,
	NUM_FLOAT
};

// var_val status
#define VV_LINE_UNKNOWN		0
#define VV_LINE_BROKEN		1
#define VV_LINE_COMMENT		2
#define VV_LINE_BLANK		3
#define VV_LINE_NOATT		4
#define VV_LINE_ATTVAL		5
#define VV_LINE_AOVERLEN	6
#define VV_LINE_VOVERLEN	7


// var_val flags
#define	VV_NO_EQUAL			0x01	// don't check for equal
#define	VV_NO_SPACE			0x02	// don't check for space
#define	VV_NO_TAB			0x04	// don't check for tabs
#define	VV_NO_VALS			0x08	// there's just attributes
#define	VV_VAL_WHITESPACE	0x10	// don't clean value whitespace
#define	VV_AUTO_VAL			0x20	// put "1" as value where none present
#define VV_LOWER_ATT		0x40	// lowercase the attribute
#define VV_REMOVE_UDRSCR	0x80	// remove underscores

#define AVP_MAX_ATT			1024
#define	AVP_MAX_VAL			8192

// avoid typos
#define MILLION				1000000
#define BILLION				1000000000


#define tsll( a )			( ( 1000000000 * (int64_t) a.tv_sec ) + (int64_t) a.tv_nsec )
#define llts( a, _s )		_s.tv_sec = (time_t) ( a / 1000000000 ); _s.tv_nsec = (long) ( a % 1000000000 )
#define lltv( a, _v )		_v.tv_sec = (time_t) ( a / 1000000000 ); _v.tv_usec = (long) ( ( a % 1000000000 ) / 1000 )
#define tsdupe( a, b )		b.tv_sec = a.tv_sec; b.tv_nsec = a.tv_nsec

#define tvalts( _t )		( _t / 1000000000 )
#define tvalns( _t )		( _t % 1000000000 )


struct words_data
{
	char				*	wd[STRWORDS_MAX];
	char				*	end;
	int						len[STRWORDS_MAX];
	int						end_len;
	int						in_len;
	int						wc;
};


struct string_buffer
{
	char				*	space;
	char				*	buf;
	uint32_t				len;
	uint32_t				sz;
};


struct lockless_counter
{
	uint64_t				count;
	uint64_t				prev;
};


struct av_pair
{
	char					att[AVP_MAX_ATT];
	char					val[AVP_MAX_VAL];
	int						alen;
	int						vlen;
	int						status;
};



// FUNCTIONS

// zero'd memory
void *allocz( size_t size );

void get_time( void );

// allocation of strings that can't be freed
char *perm_str( int len );
char *str_dup( char *src, int len );

// this can be freed
char *str_copy( char *src, int len );

// get a buffer
BUF *strbuf( uint32_t size );
int strbuf_copy( BUF *b, char *str, int len );
int strbuf_add( BUF *b, char *str, int len );

// these work as macros
#define strbuf_printf( b, ... )			b->len = snprintf( b->buf, b->sz, ##__VA_ARGS__ )
#define strbuf_aprintf( b, ... )		b->len += snprintf( b->buf + b->len, b->sz - b->len, ##__VA_ARGS__ )
#define strbuf_empty( b )				b->len = 0; b->buf[0] = '\0'
#define strbuf_chop( b )				if( b->len > 0 ) { b->len--; b->buf[b->len] = '\0'; }
#define strbuf_lastchar( b )			( ( b->len ) ? b->buf[b->len - 1] : '\0' )

// get string length, up to a maximum
int str_nlen( char *src, int max );

// processing a config line in variable/value
int var_val( char *line, int len, AVP *av, int flags );

// break up a string by delimiter
int strwords( WORDS *w, char *src, int len, char sep );

// timespec difference as a double
double ts_diff( struct timespec to, struct timespec from, double *store );

// get our uptime
double get_uptime( void );

// set an rlimit
int setlimit( int res, int64_t val );

// fetch a lockless counter
uint64_t lockless_fetch( LLCT *l );

// read a file into memory
int read_file( char *path, char **buf, int *len, size_t max, int perm, char *desc );

// handle any number type
// returns the type it thought it was
int parse_number( char *str, int64_t *iv, double *dv );

// easier av reads
#define av_int( _v )		parse_number( av->val, &(_v), NULL )
#define av_dbl( _v )		parse_number( av->val, NULL, &(_v) )

#define av_intk( _v )		av_int( _v ); _v *= 1000
#define av_intK( _v )		av_int( _v ); _v <<= 10

// hash size lookup
uint64_t hash_size( char *str );


#endif
