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
* pmet/conf.c - functions to provide prometheus metrics config            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


PMET_TYPE pmet_types[PMET_TYPE_MAX] =
{
	{
		.type = PMET_TYPE_UNTYPED,
		.name = "untyped",
		.rndr = &pmet_counter_render,
		.valu = &pmet_counter_value
	},
	{
		.type = PMET_TYPE_COUNTER,
		.name = "counter",
		.rndr = &pmet_counter_render,
		.valu = &pmet_counter_value
	},
	{
		.type = PMET_TYPE_GAUGE,
		.name = "gauge",
		.rndr = &pmet_gauge_render,
		.valu = &pmet_gauge_value
	},
	{
		.type = PMET_TYPE_SUMMARY,
		.name = "summary",
		.rndr = &pmet_summary_render,
		.valu = &pmet_summary_value
	},
	{
		.type = PMET_TYPE_HISTOGRAM,
		.name = "histogram",
		.rndr = &pmet_histogram_render,
		.valu = &pmet_histogram_value
	}
};


PMET_CTL *_pmet = NULL;



PMET_CTL *pmet_config_defaults( void )
{
	PMET_CTL *p = (PMET_CTL *) mem_perm( sizeof( PMET_CTL ) );
	int v;

	p->period = DEFAULT_PMET_PERIOD_MSEC;
	p->enabled = 0;

	v = 1;
	p->srclookup = string_store_create( 0, "nano", &v, 0 );
	p->metlookup = string_store_create( 0, "tiny", &v, 0 );

	// give ourselves half a meg
	p->page = strbuf( DEFAULT_PMET_BUF_SZ );
	strbuf_copy( p->page, "# Not ready yet.\n", 0 );

	if( ( v = regcomp( &(p->path_check), PMET_PATH_CHK_RGX, REG_EXTENDED|REG_ICASE|REG_NOSUB ) ) )
	{
		char *errbuf = (char *) allocz( 2048 );

		regerror( v, &(p->path_check), errbuf, 2048 );
		fatal( "Could not compile prometheus metrics path check regex: %s", errbuf );
		return NULL;
	}

	pthread_mutex_init( &(p->genlock), NULL );

	_pmet = p;

	// add in the basics
	p->shared = (PMET_SH *) mem_perm( sizeof( PMET_SH ) );
	p->shared->source = pmet_add_source( "shared" );

	return p;
}


int pmet_config_line( AVP *av )
{
	PMET_CTL *p = _pmet;
	int64_t v;
	WORDS wd;
	char *lv;
	SSTE *e;
	int i;

	if( attIs( "enable" ) )
	{
		p->enabled = config_bool( av );
	}
	else if( attIs( "period" ) || attIs( "interval" ) )
	{
		if( av_int( v ) == NUM_INVALID )
		{
			err( "Invalid prometheus metrics generation period '%s'.", av->vptr );
			return -1;
		}

		p->period = v;
	}
	else if( attIs( "disable" ) )
	{
		if( !( e = string_store_add( p->srclookup, av->vptr, av->vlen ) ) )
		{
			err( "Could not handle disabling prometheus metrics type '%s'.", av->vptr );
			return -1;
		}
		info( "Prometheus metrics type %s disabled.", av->vptr );
		e->val = 0;
	}
	else if( attIs( "labels" ) )
	{
		// comma separated key:value pairs
		strwords( &wd, av->vptr, av->vlen, ',' );
		for( i = 0; i < wd.wc; ++i )
		{
			// an extra default label, in the form of var:value
			if( !( lv = memchr( wd.wd[i], ':', wd.len[i] ) ) || !*(lv+1) )
			{
				err( "Invalid default label '%s', format is foo:bar", wd.wd[i] );
				return -1;
			}
			*lv++ = '\0';
			pmet_label_common( wd.wd[i], lv );
		}
	}
	else if( attIs( "alertPeriod" ) )
	{
		// number of seconds without a scrape
		// when we alert
		if( av_int( v ) == NUM_INVALID )
		{
			err( "Invalid prometheus metric alert frequency '%s'", av->vptr );
			return -1;
		}

		// in seconds
		p->alert_period = v;
	}
	else
		return -1;

	return 0;
}


