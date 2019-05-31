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





struct pmet_control
{
	PMSRC			*	sources;
	PMSRC			*	shared;

	SSTR			*	lookup;

	PMET_LBL		*	common;

	regex_t				path_check;

	int64_t				outsz;
	int64_t				timestamp;

	int64_t				period;
	int					enabled;
	int					scount;
};



// if an item is provided, the created label will be added to it
PMET_LBL *pmet_label_create( char *name, char *val, PMET *item );

// pass an app-common label in
int pmet_label_common( char *name, char *valptr );

// clone a list of labels
// max -1 means all, no matter the length
PMET_LBL *pmet_label_clone( PMET_LBL *in, int max );


// wrapper fns
int pmet_set( PMET *item, int count, double *vals, int copy );
int pmet_setv( PMET *item, int count, ... );
int pmet_max_vals( PMET *item, int64_t max ); // only meaningful for summary
PMET *pmet_create( int type, char *path, char *help, int gentype, void *genptr, void *genarg );
PMET *pmet_clone( PMET *item, void *genptr, void *genarg );

int pmet_value( PMET *item, double value, int set );

// gauge interface
#define pmet_val_add( _i, _v )			pmet_value( _i, _v, 0 )
#define pmet_val_sub( _i, _v )			pmet_value( _i, ( -1 * _v ), 0 )
#define pmet_val_set( _i, _v )			pmet_value( _i, _v, 1 )



// pmet.c

int pmet_add_item( PMSRC *src, PMET *item );
int pmet_add_item_by_name( char *src, PMET *item );

http_callback pmet_source_control;
http_callback pmet_source_list;


PMSRC *pmet_add_source( char *name, int sz );


int pmet_init( void );
PMET_CTL *pmet_config_defaults( void );
int pmet_config_line( AVP *av );


#endif
