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

#define DEFAULT_PMET_BUF_SZ				0x7f00		// just under 32k
#define DEFAULT_PMET_PERIOD_MSEC		10000


// minimum 2 chars (up), allows words separated by single underscores
// must begin with a letter, may not end with an underscore, may be a
// single word
#define PATH_CHK_RGX		"(([a-z][a-z0-9]*_)+|[a-z][a-z0-9]*)[a-z0-9]+"



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
	PMET_GEN_IVAL = 0,
	PMET_GEN_DVAL,
	PMET_GEN_LLCT,
	PMET_GEN_FN,
	PMET_GEN_MAX
};


struct pmet_type_info
{
	int8_t				type;
	char			*	name;
	pmet_render_fn	*	rndr;
	pmet_value_fn	*	valu;
};

extern PMET_TYPE pmet_types[PMET_TYPE_MAX];


// label set
struct pmet_label
{
	PMET_LBL		*	next;
	char			*	name;
	char			**	valptr;
};


// note, we don't keep the actual values against
// the values item - it is of the same size as
// quantiles and is where each quantile point is
// stored
struct pmet_summary
{
	double			*	quantiles;
	char			**	qvals;
	PMET_LBL		*	labels;

	double			*	values;
	int64_t				max;

	int64_t				count;
	double				sum;

	int					qcount;
};

// this would likely need constant update to be
// useful; a function to do that is provided
struct pmet_histogram
{
	double			*	buckets;
	char			**	bvals;
	PMET_LBL		**	labels;
	int64_t			*	counts;

	int64_t				count;
	double				sum;

	int					bcount;
};



union pmet_value
{
	double				dval;
	PMET_SUM		*	summ;
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


struct pmet_control
{
	PMSRC			*	sources;
	PMSRC			*	shared;

	SSTR			*	lookup;

	regex_t				path_check;
	char			*	plus_inf;

	int64_t				outsz;
	int64_t				timestamp;

	int64_t				period;
	int					enabled;
	int					scount;
};



// histogram

pmet_render_fn pmet_histogram_render;
pmet_value_fn pmet_histogram_value;

int pmet_histogram_set( PMET *item, int count, double *vals, int copy );
int pmet_histogram_setv( PMET *item, int count, ... );


// summary

pmet_render_fn pmet_summary_render;
pmet_value_fn pmet_summary_value;

int pmet_summary_set( PMET *item, int count, double *quantiles, int copy );
int pmet_summary_setv( PMET *item, int count, ... );


// label

int pmet_label_render( BUF *b, int count, ... );

// if an item is provided, the created label will be added to it
PMET_LBL *pmet_label_create( char *name, char **valptr, PMET *item );


// item

PMET *pmet_item_create( int type, char *path, char *help, int gentype, void *genptr, void *genarg );

// if genptr or genarg are NULL, the one from the source is used
// all labels are *cloned*
PMET *pmet_item_clone( PMET *item, void *genptr, void *genarg );


// wrapper fn
int pmet_value( PMET *item, double value );



// pmet.c

int pmet_add_item( PMSRC *src, PMET *item );
int pmet_gen_item( PMET *item );

http_callback pmet_source_control;
http_callback pmet_source_list;

loop_call_fn pmet_pass;
throw_fn pmet_run;

int pmet_init( void );

PMSRC *pmet_add_source( pmet_fn *fp, char *name, void *arg, int sz );

PMET_CTL *pmet_config_defaults( void );
int pmet_config_line( AVP *av );


#endif
