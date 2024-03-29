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
* utils.h - routine declations and utility structures                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef SHARED_UTILS_H
#define SHARED_UTILS_H



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
#define	VV_NO_EQUAL			0x0001	// don't check for equal
#define	VV_NO_SPACE			0x0002	// don't check for space
#define	VV_NO_TAB			0x0004	// don't check for tabs
#define	VV_NO_VALS			0x0008	// there's just attributes
#define	VV_VAL_WHITESPACE	0x0010	// don't clean value whitespace
#define	VV_AUTO_VAL			0x0020	// put "1" as value where none present
#define VV_LOWER_ATT		0x0040	// lowercase the attribute
#define VV_REMOVE_UDRSCR	0x0080	// remove underscores

#define AVP_MAX_ATT			1024
#define	AVP_MAX_VAL			8192

// avoid typos
#define MILLION				1000000L
#define MILLIONF			1000000.0
#define BILLION				1000000000L
#define BILLIONF			1000000000.0


#define tsll( a )			( ( BILLION * (int64_t) a.tv_sec  ) + (int64_t) a.tv_nsec )
#define tsllp( a )			( ( BILLION * (int64_t) a->tv_sec ) + (int64_t) a->tv_nsec )

#define llts( a, _s )		_s.tv_sec  = (time_t) ( a / BILLION ); _s.tv_nsec  = (long)   ( a % BILLION )
#define lltsp( a, _s )		_s->tv_sec = (time_t) ( a / BILLION ); _s->tv_nsec = (long)   ( a % BILLION )

#define lltv( a, _v )		_v.tv_sec = (time_t) ( a / BILLION ); _v.tv_usec = (long) ( ( a % BILLION ) / 1000 )
#define tsdupe( a, b )		b.tv_sec = a.tv_sec; b.tv_nsec = a.tv_nsec

#define tvalts( _t )		( _t / BILLION )
#define tvalns( _t )		( _t % BILLION )
#define timedbl( _t )		( ( (double) tvalts( _t ) ) + ( (double) tvalns( _t ) / BILLIONF ) )



struct lockless_counter
{
	uint64_t				count;
	uint64_t				prev;
};


struct av_pair
{
	char					att[AVP_MAX_ATT];
	char					val[AVP_MAX_VAL];
	char				*	aptr;
	char				*	vptr;
	int						alen;
	int						vlen;
	int						status;
	int						blank;
	int						doFree;
};



// FUNCTIONS

void get_time( void );
int64_t get_time64( void );

// get a number from 0 <= x < n
int64_t get_rand( int64_t n );

// sleep some microseconds
// wraps nanosleep
void microsleep( int64_t usec );

// parse a timespan, eg 10s, 40w, 2mo
int time_span_usec( const char *str, int64_t *usec );

// parse a timestamp, get usec
// uses 250* thresholds
int time_usec( const char *str, int64_t *usec );

// get a percentage - 1-100
int8_t percent( void );

// processing a config line in variable/value
int var_val( char *line, int len, AVP *av, int flags );

// timespec difference as a double
double ts_diff( struct timespec to, struct timespec from, double *store );

// get our uptime
double get_uptime( void );
int get_uptime_msec( char *buf, int len );
time_t get_uptime_sec( void );

// set an rlimit
int setlimit( int res, int64_t val );

// fetch a lockless counter
uint64_t lockless_fetch( LLCT *l );

// read a file into memory
int read_file( const char *path, char **buf, int *len, int perm, const char *desc );

// handle any number type
// returns the type it thought it was
int parse_number( const char *str, int64_t *iv, double *dv );

// parse a file for json
json_object *parse_json_file( FILE *fh, const char *path );

// create a json result and put it in a buffer
void create_json_result( BUF *buf, int ok, const char *fmt, ... );


// is this an http url
enum str_url_types
{
	STR_URL_NO = 0,
	STR_URL_YES,
	STR_URL_SSL
};
int is_url( const char *str );


// easier av reads
#define av_int( _v )		parse_number( av->vptr, &(_v), NULL )
#define av_dbl( _v )		parse_number( av->vptr, NULL, &(_v) )

#define av_intk( _v )		av_int( _v ); _v *= 1000
#define av_intK( _v )		av_int( _v ); _v <<= 10

#define av_copy( _v )		str_copy( _v->vptr, _v->vlen )
#define av_copyp( _v )		str_perm( _v->vptr, _v->vlen )


// flags field laziness
#define flag_add( _i, _f )	_i |=  _f
#define flag_rmv( _i, _f )	_i &= ~_f
#define flag_has( _i, _f )	( _i & _f )

#define flagf_add( _o, _f )	flag_add( (_o->flags), _f )
#define flagf_rmv( _o, _f )	flag_rmv( (_o->flags), _f )
#define flagf_has( _o, _f )	flag_has( (_o->flags), _f )
#define flagf_val( _o, _f )	( ( flagf_has( _o, _f ) ) ? 1 : 0 )

#define flagf_set( _o, _f, _b )	if( _b ) flagf_add( _o, _f ); else flagf_rmv( _o, _f )

#define runf_add( _f )		flag_add( _proc->run_flags, _f )
#define runf_rmv( _f )		flag_rmv( _proc->run_flags, _f )
#define runf_has( _f )		flag_has( _proc->run_flags, _f )

#define RUN_START( )        runf_add( RUN_LOOP )
#define RUN_STOP( )         runf_rmv( RUN_LOOP )
#define RUNNING( )          runf_has( RUN_LOOP )

#define plural( _d )		( ( _d == 1 ) ? "" : "s" )


// json shortcuts
#define json_fetch( _obj, _key, _type )         json_object_get_##_type( json_object_object_get( _obj, _key ) )
#define json_insert( _obj, _key, _type, _item ) json_object_object_add( _obj, _key, json_object_new_##_type( _item ) )
#define json_append( _arr, _type, _item )       json_object_array_add( _arr, json_object_new_##_type( _item ) )

// hash size lookup
uint64_t hash_size( const char *str );


#endif
