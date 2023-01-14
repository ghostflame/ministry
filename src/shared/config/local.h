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
