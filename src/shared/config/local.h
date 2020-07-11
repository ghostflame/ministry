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
CCTXT *config_make_context( const char *path, WORDS *w );
int config_source_dupe( const char *path );


int config_handle_dir( const char *path, WORDS *w );

void config_set_main_file( const char *path );
void config_choose_section( CCTXT *c, const char *section );

void config_set_main_file( const char *path );
void config_set_env_prefix( const char *prefix );
void config_set_suffix( const char *suffix );

#endif
