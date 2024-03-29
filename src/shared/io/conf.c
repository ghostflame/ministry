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
* io/conf.c - network i/o configuration functions                         *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


IO_CTL *_io = NULL;




IO_CTL *io_config_defaults( void )
{
	_io = (IO_CTL *) mem_perm( sizeof( IO_CTL ) );
	_io->lock_bits = IO_BLOCK_BITS;
	_io->send_usec = IO_SEND_USEC;
	_io->rc_msec   = IO_RECONN_DELAY;

	return _io;
}


int io_config_line( AVP *av )
{
	int64_t i;

	if( attIs( "sendUsec" ) )
	{
		if( av_int( i ) == NUM_INVALID )
		{
			err( "Invalid send usec value: %s", av->vptr );
			return -1;
		}

		if( i < 500 || i > 5000000 )
			warn( "Recommended send usec values are 500 <= x <= 5000000." );

		_io->send_usec = (int32_t) i;
	}
	else if( attIs( "sendMsec" ) )
	{
		if( av_int( i ) == NUM_INVALID )
		{
			err( "Invalid send msec value: %s", av->vptr );
			return -1;
		}
		if( i < 1 || i > 5000 )
			warn( "Recommended send msec values are 1 <= x <= 5000." );

		_io->send_usec = (int32_t) ( 1000 * i );
	}
	else if( attIs( "reconnectMsec" ) || attIs( "reconnMsec" ) )
	{
		if( av_int( i ) == NUM_INVALID )
		{
			err( "Invalid reconnect msec value: %s", av->vptr );
			return -1;
		}

		if( i < 20 || i > 50000 )
			warn( "Recommended reconnect msec values are 20 <= x <= 50000." );

		_io->rc_msec = (int32_t) i;
	}
	else if( attIs( "bufLockBits" ) )
	{
		if( av_int( i ) == NUM_INVALID )
		{
			err( "Invalid buf lock bits value: %s", av->vptr );
			return -1;
		}

		if( i < 3 || i > 12 )
			warn( "Recommended buf lock bits values are 3 <= x <= 12." );

		_io->rc_msec = (uint64_t) i;
	}
	else
		return -1;

	return 0;
}

