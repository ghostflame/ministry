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
* config.c - read config files and create config object                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#include "local.h"



CCTXT *ctxt_top = NULL;
CCTXT *context  = NULL;


CCTXT *config_make_context( const char *path, WORDS *w )
{
	CCTXT *ctx, *parent = context;
	int i;

	ctx = (CCTXT *) allocz( sizeof( CCTXT ) );
	snprintf( ctx->source, 4096, "%s", path );

	// copy in our arguments
	if( w && w->wc > 1 )
	{
		// when we get a w, the first arg is the path
		ctx->argc = w->wc - 1;
		ctx->argl = (int *)   allocz( ctx->argc * sizeof( int ) );
		ctx->argv = (char **) allocz( ctx->argc * sizeof( char * ) );
		for( i = 1; i < w->wc; ++i )
		{
			ctx->argv[i-1] = str_copy( w->wd[i], w->len[i] );
			ctx->argl[i-1] = w->len[i];
		}
	}

	if( parent )
	{
		ctx->parent = parent;

		if( parent->children )
			parent->children->next = ctx;
		parent->children = ctx;

		// inherit the section we were in
		ctx->section = parent->section;

		// and if we don't have any of our own, inherit args
		if( !ctx->argc && parent->argc )
		{
			ctx->argc = parent->argc;
			ctx->argl = parent->argl;
			ctx->argv = parent->argv;
		}
	}
	else
		config_choose_section( ctx, "main" );

	// set the global on the first call
	if( !ctxt_top )
		ctxt_top = ctx;

	return ctx;
}



int config_source_dupe( const char *path )
{
	CCTXT *c = context;

	while( c )
	{
		if( !strcmp( path, c->source ) )
			return 1;

		c = c->parent;
	}

	return 0;
}


