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



struct pmet_source
{
	PMSRC			*	next;
	BUF				*	buf;
	pmet_fn			*	fp;
	SSTE			*	sse;
	char			*	name;
	int					nlen;
	int					last_ct;
};


struct pmet_control
{
	PMSRC			*	sources;
	PMSRC			*	shared;

	SSTR			*	lookup;

	int64_t				outsz;
	int64_t				timestamp;

	int64_t				period;
	int					enabled;
	int					scount;
};


http_callback pmet_source_control;
http_callback pmet_source_list;

loop_call_fn pmet_pass;
throw_fn pmet_run;

int pmet_init( void );

PMSRC *pmet_add_source( pmet_fn *fp, char *name, void *arg, int sz );

PMET_CTL *pmet_config_defaults( void );
int pmet_config_line( AVP *av );


#endif
