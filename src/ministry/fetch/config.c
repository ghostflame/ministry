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
* fetch/fetch.c - handles fetching metrics                                *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "ministry.h"





FTCH_CTL *fetch_config_defaults( void )
{
	FTCH_CTL *fc = (FTCH_CTL *) mem_perm( sizeof( FTCH_CTL ) );
	return fc;
}


static FETCH __fetch_config_tmp;
static int __fetch_config_state = 0;

int fetch_config_line( AVP *av )
{
	FETCH *n, *f = &__fetch_config_tmp;
	int64_t v;
	DTYPE *d;
	int i;

	if( !__fetch_config_state )
	{
		if( f->name )
			free( f->name );
		memset( f, 0, sizeof( FETCH ) );
		f->name = str_copy( "as-yet-unnamed", 0 );
		f->ch = (CURLWH *) allocz( sizeof( CURLWH ) );
		f->bufsz = DEFAULT_FETCH_BUF_SZ;
		f->metdata = (MDATA *) allocz( sizeof( MDATA ) );
		f->metdata->hsz = METR_HASH_SZ;
		f->enabled = 1;

		// set validate by default if we are given it on
		// the command line.  It principally affects config
		// fetch, but we use it here too
		if( chkcfFlag( SEC_VALIDATE ) && !chkcfFlag( SEC_VALIDATE_F ) )
			setCurlF( f->ch, VALIDATE );
	}

	if( attIs( "name" ) )
	{
		free( f->name );
		f->name = str_copy( av->vptr, av->vlen );
		__fetch_config_state = 1;
	}
	else if( attIs( "host" ) )
	{
		if( f->remote )
		{
			err( "Fetch block %s already has a remote host: '%s'", f->name, f->remote );
			return -1;
		}

		f->remote = av_copyp( av );
		__fetch_config_state = 1;
	}
	else if( attIs( "port" ) )
	{
		if( parse_number( av->vptr, &v, NULL ) == NUM_INVALID )
		{
			err( "Invalid remote port for fetch block %s: '%s'", f->name, av->vptr );
			return -1;
		}

		f->port = (uint16_t) v;
		__fetch_config_state = 1;
	}
	else if( attIs( "path" ) )
	{
		if( f->path )
		{
			err( "Fetch block %s already had a path.", f->name );
			return -1;
		}

		if( av->vptr[0] == '/' )
			f->path = av_copyp( av );
		else
		{
			f->path = mem_perm( av->vlen + 2 );
			snprintf( f->path, av->vlen + 2, "/%s", av->vptr );
		}
		__fetch_config_state = 1;
	}
	else if( attIs( "ssl" ) || attIs( "tls" ) )
	{
		if( config_bool( av ) )
			setCurlF( f->ch, SSL );
		else
			cutCurlF( f->ch, SSL );

		__fetch_config_state = 1;
	}
	else if( attIs( "validate" ) )
	{
		if( config_bool( av ) )
			setCurlF( f->ch, VALIDATE );
		else
		{
			if( chkcfFlag( SEC_VALIDATE ) && !chkcfFlag( SEC_VALIDATE_F ) )
			{
				err( "Cannot disable validation on fetch block %s - cmd-line flag -T is set, but -W is not.",
					f->name );
				return -1;
			}
			cutCurlF( f->ch, VALIDATE );
		}
		// absent means use default

		__fetch_config_state = 1;
	}
	else if( attIs( "prometheus" ) )
	{
		f->metrics = config_bool( av );
		__fetch_config_state = 1;
	}
	else if( attIs( "profile" ) )
	{
		if( f->profile )
		{
			err( "Fetch %s already has profile %s.", f->name, f->profile );
			return -1;
		}

		f->profile = av_copyp( av );
		f->metrics = 1;
		__fetch_config_state = 1;
	}
	else if( attIs( "typehash" ) )
	{
		if( parse_number( av->vptr, &v, NULL ) == NUM_INVALID )
		{
			err( "Invalid metric type hash size: %s", av->vptr );
			return -1;
		}

		f->metdata->hsz = (uint64_t) v;
		__fetch_config_state = 1;
	}
	else if( attIs( "type" ) )
	{
		f->dtype = NULL;

		for( i = 0, d = (DTYPE *) data_type_defns; i < DATA_TYPE_MAX; ++i, ++d )
			if( !strcasecmp( d->name, av->vptr ) )
			{
				f->dtype = d;
				break;
			}

		if( !f->dtype )
		{
			err( "Type '%s' not recognised.", av->vptr );
			return -1;
		}
		if( f->metrics )
			warn( "Data type set to '%s' but this is a prometheus source.", f->dtype->name );

		__fetch_config_state = 1;
	}
	else if( attIs( "period" ) )
	{
		if( parse_number( av->vptr, &v, NULL ) == NUM_INVALID )
		{
			err( "Invalid fetch period in block %s: %s", f->name, av->vptr );
			return -1;
		}

		f->period = v;
		__fetch_config_state = 1;
	}
	else if( attIs( "revalidate" ) )
	{
		if( parse_number( av->vptr, &v, NULL ) == NUM_INVALID )
		{
			err( "Invalid DNS revalidation period in block %s: %s", f->name, av->vptr );
			return -1;
		}
		f->revalidate = v;
		__fetch_config_state = 1;
	}
	else if( attIs( "offset" ) )
	{
		if( parse_number( av->vptr, &v, NULL ) == NUM_INVALID )
		{
			err( "Invalid fetch offset in block %s: %s", f->name, av->vptr );
			return -1;
		}

		f->offset = v;
		__fetch_config_state = 1;
	}
	else if( attIs( "buffer" ) )
	{
		if( parse_number( av->vptr, &v, NULL ) == NUM_INVALID )
		{
			err( "Invalid buffer size for block %s: %s", f->name, av->vptr );
			return -1;
		}

		// cheating - low values are bit shift values
		if( v < 28 )
			v = 1 << v;

		if( v < 0x4000 )	// 16k minimum
		{
			warn( "Fetch block %s buffer is too small at %ld bytes - minimum 16k", f->name, v );
			v = 0x4000;
		}

		f->bufsz = v;
		__fetch_config_state = 1;
	}
	else if( attIs( "attribute" ) )
	{
		// prometheus attribute map
		__fetch_config_state = 1;
	}
	else if( attIs( "enable" ) )
	{
		f->enabled = config_bool( av );
		__fetch_config_state = 1;
	}
	else if( attIs( "done" ) )
	{
		if( !f->remote )
		{
			err( "Fetch block %s has no target host!", f->name );
			return -1;
		}
		if( !f->metrics && !f->dtype )
		{
			err( "Fetch block %s has no type!", f->name );
			return -1;
		}

		debug( "Adding fetch block: %s", f->name );

		// copy it
		n = (FETCH *) allocz( sizeof( FETCH ) );
		*n = *f;

		// zero the static struct
		memset( f, 0, sizeof( FETCH ) );
		__fetch_config_state = 0;

		// link it up
		n->next = ctl->fetch->targets;
		ctl->fetch->targets = n;
		++(ctl->fetch->fcount);
	}
	else
		return -1;

	return 0;
}

