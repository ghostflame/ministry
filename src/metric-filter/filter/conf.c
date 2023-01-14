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
* filter/conf.c - filtering functions                                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

FLT_CTL *_filt = NULL;


FLT_CTL *filter_config_defaults( void )
{
	_filt = (FLT_CTL *) mem_perm( sizeof( FLT_CTL ) );

	_filt->filter_dir = str_copy( DEFAULT_FILTER_DIR, 0 );
	_filt->generation = 1;
	_filt->close_conn = 1;
	_filt->flush_max  = FILTER_DEFAULT_FLUSH * MILLION;
	_filt->chg_delay  = FILTER_DEFAULT_DELAY;

	return _filt;
}

int filter_config_line( AVP *av )
{
	FLT_CTL *f = _filt;
	int64_t v;
	char *rp;

	if( attIs( "filters" ) || attIs( "filterDir" ) )
	{
		if( !( rp = realpath( av->vptr, NULL ) ) )
		{
			err( "Cannot find real path for filter dir '%s' -- %s",
				av->vptr, Err );
			return -1;
		}
		free( f->filter_dir );
		f->filter_dir = str_copy( rp, 0 );
		info( "Filter directory: %s", f->filter_dir );
	}
	else if( attIs( "flushMax" ) )
	{
		if( av_int( v ) == NUM_INVALID )
		{
			err( "Invalid flush max time in msec: %s", av->vptr );
			return -1;
		}

		f->flush_max = v * MILLION;
	}
	else if( attIs( "changeDelay" ) )
	{
		if( av_int( v ) == NUM_INVALID )
		{
			err( "Invalid change delay time in msec: %s", av->vptr );
			return -1;
		}

		f->chg_delay = v;
	}
	else if( attIs( "disconnect" ) )
	{
		f->close_conn = config_bool( av );
	}
	else
		return -1;

	return 0;
}
