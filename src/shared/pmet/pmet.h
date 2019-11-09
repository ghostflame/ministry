/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* pmet.h - definitions and structs for prometheus metrics endpoint        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_PMET_H
#define SHARED_PMET_H


enum pmet_type_vals
{
	PMET_TYPE_UNTYPED = 0,
	PMET_TYPE_COUNTER,
	PMET_TYPE_GAUGE,
	PMET_TYPE_SUMMARY,
	PMET_TYPE_HISTOGRAM,
	PMET_TYPE_MAX
};

enum pmet_gen_types
{
	PMET_GEN_NONE = 0,  // fed, not generated
	PMET_GEN_IVAL,
	PMET_GEN_DVAL,
	PMET_GEN_LLCT,
	PMET_GEN_FN,
	PMET_GEN_MAX
};



// label set
struct pmet_label
{
	PMET_LBL		*	next;
	char			*	name;
	char			*	val;
};


struct pmet_shared
{
	PMETS			*	source;

	PMETM			*	upmet;
	PMET			*	uptime;

	PMETM			*	memmet;
	PMET			*	mem;

	PMETM			*	cfgmet;
	PMET			*	cfgChg;

	PMETM			*	logmet;
	PMET			*	logs[LOG_LEVEL_MAX];
};


struct pmet_control
{
	PMETS			*	sources;
	PMETM			*	metrics;

	SSTR			*	srclookup;
	SSTR			*	metlookup;

	PMET_LBL		*	common;
	BUF				*	page;

	PMET_SH			*	shared;

	regex_t				path_check;
	pthread_mutex_t		genlock;

	int64_t				timestamp;

	int64_t				period;
	int					enabled;
};


#define pmet_genlock( )			pthread_mutex_lock(   &(_proc->pmet->genlock) )
#define pmet_genunlock( )		pthread_mutex_unlock( &(_proc->pmet->genlock) )

#define pmets_enabled( _s )		( _s->sse->val )


// if an item is provided, the created label will be added to it
PMET_LBL *pmet_label_create( char *name, char *val );
PMET_LBL *pmet_label_words( WORDS *w );

int pmet_label_apply_to( PMET_LBL *list, PMETM *metric, PMET *item );
#define pmet_label_apply_item( _l, _i )		pmet_label_apply_to( _l, NULL, _i )
#define pmet_label_apply_metric( _l, _m )	pmet_label_apply_to( _l, _m, NULL )

// pass an app-common label in
int pmet_label_common( char *name, char *valptr );

// clone a list of labels
// max -1 means all, no matter the length
PMET_LBL *pmet_label_clone( PMET_LBL *in, int max, PMET_LBL *except );


// wrapper fns
int pmet_set( PMET *item, int count, double *vals, int copy );
int pmet_setv( PMET *item, int count, ... );
int pmet_max_vals( PMET *item, int64_t max ); // only meaningful for summary

// full interface
PMET *pmet_create_name( char *metric, char *source, int gentype, void *genptr, pmet_gen_fn *fp, void *genarg );
PMET *pmet_create_gen( PMETM *metric, PMETS *source, int gentype, void *genptr, pmet_gen_fn *fp, void *genarg );
PMET *pmet_create( PMETM *metric, PMETS *source );
PMET *pmet_clone_gen( PMET *item, PMETS *source, void *genptr, pmet_gen_fn *fp, void *genarg );
PMET *pmet_clone( PMET *item );
PMET *pmet_clone_vary( PMET *item, PMET_LBL *lbl ); // clones but for this label

// new metric
PMETM *pmet_new( int type, char *path, char *help );

// set is only examined for gauges
int pmet_value_set( PMET *item, double value, int set );
int pmet_value( PMET *item, double value );

// gauge interface
#define pmet_val_add( _i, _v )			pmet_value_set( _i, _v, 0 )
#define pmet_val_sub( _i, _v )			pmet_value_set( _i, ( -1 * _v ), 0 )
#define pmet_val_set( _i, _v )			pmet_value_set( _i, _v, 1 )

// counter interface
#define pmet_incr( _i )					pmet_value( _i, 1 )
#define pmet_plus( _i, _v )				pmet_value( _i, v )



// metric.c
PMETM *pmet_metric_find( char *name );


// pmet.c

http_callback pmet_source_control;
http_callback pmet_source_list;


PMETS *pmet_add_source( char *name );
void pmet_report( BUF *into );


int pmet_init( void );
PMET_CTL *pmet_config_defaults( void );
int pmet_config_line( AVP *av );


#endif
