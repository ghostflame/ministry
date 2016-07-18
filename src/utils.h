/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* utils.h - routine declations and utility structures                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef MINISTRY_UTILS_H
#define MINISTRY_UTILS_H

#define DEFAULT_PID_FILE	"/var/run/ministry/ministry.pid"


// for fast string allocation, free not possible
#define PERM_SPACE_BLOCK	0x1000000	// 1M

// for breaking up strings on a delimiter
#define STRWORDS_MAX		339			// makes for a 4k struct

// var_val status
#define VV_LINE_UNKNOWN		0
#define VV_LINE_BROKEN		1
#define VV_LINE_COMMENT		2
#define VV_LINE_BLANK		3
#define VV_LINE_NOATT		4
#define VV_LINE_ATTVAL		5

// var_val flags
#define	VV_NO_EQUAL			0x01	// don't check for equal
#define	VV_NO_SPACE			0x02	// don't check for space
#define	VV_NO_TAB			0x04	// don't check for tabs
#define	VV_NO_VALS			0x08	// there's just attributes
#define	VV_VAL_WHITESPACE	0x10	// don't clean value whitespace
#define	VV_AUTO_VAL			0x20	// put "1" as value where none present
#define VV_LOWER_ATT		0x40	// lowercase the attribute



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


struct lockless_counter
{
	uint64_t				count;
	uint64_t				prev;
};


struct av_pair
{
	char				*	att;
	char				*	val;
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

// get string length, up to a maximum
int str_nlen( char *src, int max );

// processing a config line in variable/value
int var_val( char *line, int len, AVP *av, int flags );

// break up a string by delimiter
int strwords( WORDS *w, char *src, int len, char sep );

// handle pidfile
void pidfile_write( void );
void pidfile_remove( void );

// proper float summation
void kahan_summation( double *list, int len, double *sum );

// timespec difference as a double
double ts_diff( struct timespec to, struct timespec from, double *store );

// set an rlimit
int setlimit( int res, int64_t val );

// fetch a lockless counter
uint64_t lockless_fetch( LLCT *l );

#endif
