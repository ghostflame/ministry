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
* pmet/local.h - local defs and structures                                *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_PMET_LOCAL_H
#define SHARED_PMET_LOCAL_H


#define DEFAULT_PMET_BUF_SZ				0x7ffff		// just under 512k
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
	void			*	in;
};


#define lock_pmetm( _m )	pthread_mutex_lock(   &(_m->lock) )
#define unlock_pmetm( _m )	pthread_mutex_unlock( &(_m->lock) )


#define lock_pmet( _p )		if( _p->gtype == PMET_GEN_NONE ) { pthread_mutex_lock(   &(_p->lock) ); }
#define unlock_pmet( _p )	if( _p->gtype == PMET_GEN_NONE ) { pthread_mutex_unlock( &(_p->lock) ); }



struct pmet_metric
{
	PMETM			*	next;
	PMET_TYPE		*	type;
	char			*	path;
	char			*	help;
	PMET_LBL		*	labels;
	SSTE			*	sse;

	PMET			*	items;

	int16_t				plen;

	pthread_mutex_t		lock;
};



// a simple fetch item, nice and predictable
struct pmet_item
{
	PMET			*	next;
	PMETM			*	metric;
	PMETS			*	source;

	PMET_GEN			gen;
	void			*	garg;
	PMET_LBL		*	labels;

	PMET_VAL			value;
	int64_t				count;

	int8_t				gtype;
	int8_t				type;

	pthread_mutex_t		lock;
};




struct pmet_source
{
	PMETS			*	next;
	SSTE			*	sse;
	char			*	name;
	int					nlen;
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

int pmet_item_render( int64_t mval, BUF *b, PMET *item, PMET_LBL *with );
int pmet_item_gen( int64_t mval, PMET *item );

PMET *pmet_item_create( PMETM *metric, PMETS *source, int gentype, void *genptr, pmet_gen_fn *fp, void *genarg );
// if genptr or genarg are NULL, the one from the source is used
// all labels are *cloned*
PMET *pmet_item_clone( PMET *item, PMETS *source, void *genptr, pmet_gen_fn *fp, void *genarg, PMET_LBL *lbl );

// metric
int pmet_metric_render( int64_t mval, BUF *b, PMETM *metric );
int pmet_metric_gen( int64_t mval, PMETM *metric );
PMETM *pmet_metric_create( int type, char *path, char *help );
PMETM *pmet_metric_find( char *name );



// run control
pmet_gen_fn pmet_get_uptime;
pmet_gen_fn pmet_get_memory;

int pmet_gen_item( PMET *item );
loop_call_fn pmet_pass;
throw_fn pmet_run;



#endif

