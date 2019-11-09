/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* filter/conf.c - filtering functions                                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

FLT_CTL *_filt = NULL;


FLT_CTL *filter_config_defaults( void )
{
	_filt = (FLT_CTL *) allocz( sizeof( FLT_CTL ) );

	_filt->filter_dir = str_copy( DEFAULT_FILTER_DIR, 0 );
	_filt->generation = 1;

	return _filt;
}

int filter_config_line( AVP *av )
{
	FLT_CTL *f = ctl->filt;
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
	else
		return -1;

	return 0;
}
