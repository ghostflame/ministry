/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* config/local.h - defines local config structure                         * 
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef SHARED_CONFIG_LOCAL_H
#define SHARED_CONFIG_LOCAL_H

#define CONF_SECT_MAX			128
#define CFG_VV_FLAGS			(VV_AUTO_VAL|VV_LOWER_ATT|VV_REMOVE_UDRSCR)


#include "shared.h"

struct config_section
{
	char				*	name;
	conf_line_fn		*	fp;
	int						section;
};


struct config_file
{
	CFILE				*	next;
	char				*	fpath;
	int64_t					mtime;
	int						fcount;
};


struct config_context
{
	CCTXT				*	next;
	CCTXT				*	children;
	CCTXT				*	parent;
	CSECT				*	section;

	char					source[4096];
	char				**	argv;
	int						lineno;
	int						is_url;
	int						is_ssl;
	int					*	argl;
	int						argc;
};



extern CCTXT *context;
extern CSECT config_sections[CONF_SECT_MAX];


// context
CCTXT *config_make_context( char *path, WORDS *w );
int config_source_dupe( char *path );




void config_choose_section( CCTXT *c, char *section );

void config_set_main_file( char *path );
void config_set_env_prefix( char *prefix );
void config_set_suffix( char *suffix );

#endif
