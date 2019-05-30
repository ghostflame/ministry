/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* pmet/local.h - local defs and structures                                *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_PMET_LOCAL_H
#define SHARED_PMET_LOCAL_H


#define DEFAULT_PMET_BUF_SZ				0x7f00		// just under 32k
#define DEFAULT_PMET_PERIOD_MSEC		10000


// minimum 2 chars (up), allows words separated by single underscores
// must begin with a letter, may not end with an underscore, may be a
// single word
#define PMET_PATH_CHK_RGX		"(([a-z][a-z0-9]*_)+|[a-z][a-z0-9]*)[a-z0-9]+"

#include "shared.h"

// some local typedefs
typedef struct pmet_summary         PMET_SUMM;
typedef struct pmet_histogram       PMET_HIST;
typedef union pmet_value            PMET_VAL;
typedef struct pmet_type_info       PMET_TYPE;
typedef union pmet_generator        PMET_GEN;

typedef double pmet_gen_fn( void *, PMET_VAL * );
typedef int pmet_render_fn( int64_t, BUF *b, PMET *, PMET_LBL * );
typedef int pmet_value_fn( PMET *, double, int );



struct pmet_type_info
{
	int8_t				type;
	char			*	name;
	pmet_render_fn	*	rndr;
	pmet_value_fn	*	valu;
};

extern PMET_TYPE pmet_types[PMET_TYPE_MAX];



// note, we don't keep the actual values against
// the values item - it is of the same size as
// quantiles and is where each quantile point is
// stored
struct pmet_summary
{
	double			*	quantiles;
	PMET_LBL		**	labels;

	double			*	values;
	double				sum;

	int64_t				max;
	int					qcount;
};

// this would likely need constant update to be
// useful; a function to do that is provided
struct pmet_histogram
{
	double			*	buckets;
	PMET_LBL		**	labels;
	int64_t			*	counts;

	double				sum;

	int					bcount;
};



union pmet_value
{
	double				dval;
	PMET_SUMM		*	summ;
	PMET_HIST		*	hist;
};



union pmet_generator
{
	int64_t			*	iptr;
	double			*	dptr;
	LLCT			*	lockless;
	pmet_gen_fn		*	genfn;
};

#define lock_pmet( _p )		pthread_mutex_lock(   &(_p->lock) )
#define unlock_pmet( _p )	pthread_mutex_unlock( &(_p->lock) )


// a simple fetch item, nice and predictable
struct pmet_item
{
	PMET			*	next;
	PMET_TYPE		*	type;
	char			*	path;
	char			*	help;
	PMET_GEN			gen;
	void			*	garg;
	PMET_LBL		*	labels;

	int8_t				lcount;
	int8_t				gtype;
	int16_t				plen;

	pthread_mutex_t		lock;

	PMET_VAL			value;
	int64_t				count;
};




struct pmet_source
{
	PMSRC			*	next;
	PMET			*	items;
	BUF				*	buf;
	pmet_fn			*	fp;
	SSTE			*	sse;
	char			*	name;
	int					nlen;
	int					icount;
	int					last_ct;
};


extern PMET_CTL *_pmet;


// histogram

pmet_render_fn pmet_histogram_render;
pmet_value_fn pmet_histogram_value;

int pmet_histogram_bucket_set( PMET *item, int count, double *buckets, int copy );


// summary

pmet_render_fn pmet_summary_render;
pmet_value_fn pmet_summary_value;

int pmet_summary_quantile_set( PMET *item, int count, double *quantiles, int copy );
int pmet_summary_make_space( PMET *item, int64_t max );


// counter
pmet_render_fn pmet_counter_render;
pmet_value_fn pmet_counter_value;


// gauge
pmet_render_fn pmet_gauge_render;
pmet_value_fn pmet_gauge_value;


// label
void pmet_label_render( BUF *b, int count, ... );
PMET_LBL **pmet_label_array( char *name, int extra, int count, double *vals );


// item
PMET *pmet_item_create( int type, char *path, char *help, int gentype, void *genptr, void *genarg );

// if genptr or genarg are NULL, the one from the source is used
// all labels are *cloned*
PMET *pmet_item_clone( PMET *item, void *genptr, void *genarg );




// run control
int pmet_gen_item( PMET *item );
loop_call_fn pmet_pass;
throw_fn pmet_run;



#endif

