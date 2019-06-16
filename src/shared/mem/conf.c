/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* mem/conf.c - memory control configuration                               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

MEM_CTL *_mem = NULL;


MEM_CTL *mem_config_defaults( void )
{
	MCHK *mc = (MCHK *) allocz( sizeof( MCHK ) );

	mc->max_kb        = DEFAULT_MEM_MAX_KB;
	mc->interval      = DEFAULT_MEM_CHECK_INTV;
	mc->psize         = getpagesize( ) >> 10;
	mc->checks        = 0;

	_mem              = (MEM_CTL *) allocz( sizeof( MEM_CTL ) );
	_mem->mcheck      = mc;
	_mem->prealloc    = DEFAULT_MEM_PRE_INTV;

	_mem->iobufs      = mem_type_declare( "iobufs", sizeof( IOBUF ), MEM_ALLOCSZ_IOBUF, IO_BUF_SZ, 1 );
	_mem->iobps       = mem_type_declare( "iobps",  sizeof( IOBP ),  MEM_ALLOCSZ_IOBP,  0, 1 );
	_mem->htreq       = mem_type_declare( "htreqs", sizeof( HTREQ ), MEM_ALLOCSZ_HTREQ, 2048, 0 );

	return _mem;
}



int mem_config_line( AVP *av )
{
	MTYPE *mt;
	int64_t t;
	char *d;
	int i;

	if( !( d = strchr( av->aptr, '.' ) ) )
	{
		// just the singles
		if( attIs( "maxMb" ) || attIs( "maxSize" ) )
		{
			av_int( _mem->mcheck->max_kb );
			_mem->mcheck->max_kb <<= 10;
			_mem->mcheck->max_set = 1;
		}
		else if( attIs( "maxKb") )
		{
			av_int( _mem->mcheck->max_kb );
			_mem->mcheck->max_set = 1;
		}
		else if( attIs( "interval" ) || attIs( "msec" ) )
			av_int( _mem->mcheck->interval );
		else if( attIs( "doChecks" ) )
			_mem->mcheck->checks = config_bool( av );
		else if( attIs( "prealloc" ) || attIs( "preallocInterval" ) )
			av_int( _mem->prealloc );
		else
			return -1;

		return 0;
	}

	*d++ = '\0';

	// after this, it's per-type control
	// so go find it.
	for( i = 0; i < MEM_TYPES_MAX; i++ )
	{
		if( !( mt = _mem->types[i] ) )
			break;

		if( attIs( mt->name ) )
			break;
	}

	if( !mt )
		return -1;

	av->alen -= d - av->aptr;
	av->aptr  = d;

	if( attIs( "block" ) )
	{
		av_int( t );
		if( t > 0 )
		{
			mt->alloc_ct = (uint32_t) t;
			debug( "Allocation block for %s set to %u", mt->name, mt->alloc_ct );
		}
	}
	else if( attIs( "prealloc" ) )
	{
		mt->prealloc = 0;
		debug( "Preallocation disabled for %s", mt->name );
	}
	else if( attIs( "threshold" ) )
	{
		av_dbl( mt->threshold );
		if( mt->threshold < 0 || mt->threshold >= 1 )
		{
			warn( "Invalid memory type threshold %f for %s - resetting to default.", mt->threshold, mt->name );
			mt->threshold = DEFAULT_MEM_PRE_THRESH;
		}
		else if( mt->threshold > 0.5 )
			warn( "Memory type threshold for %s is %f - that's quite high!", mt->name, mt->threshold );

		debug( "Mem prealloc threshold for %s is now %f", mt->name, mt->threshold );
	}
	else
		return -1;

	// done this way because GC might become a thing

	return 0;
}
