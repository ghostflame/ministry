/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* file/file.c - main file-handling functions                              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


int64_t rkv_get_size( RKHDR *hdr )
{
	int64_t sz;
	int8_t i;
	RKBKT *b;

	sz = sizeof( RKHDR );

	// the first set are just points
	b = hdr->buckets;
	sz += b->count * sizeof( PNT );

	// the rest are aggregations
	for( i = 1; i < hdr->start.buckets; ++i ) 
	{
		b = hdr->buckets + i;
		sz += b->count * sizeof( PNTA );
	}

	return sz;
}



int rkv_create( RKFL *r )
{
	char dirbuf[2048];
	int len, fd;
	RKHDR hdr;
	RKBMT *m;

	// make sure the directory is there
	memcpy( dirbuf, r->fpath, r->dlen );
	dirbuf[r->dlen] = '\0';

	if( fs_mkdir_recursive( dirbuf ) )
	{
		err( "Cannot create directories for file %s.", r->fpath );
		return -1;
	}

	// find a retention match
	for( m = _rkv->retentions; m; m = m->next )
		if( m->rgx && regex_list_test( r->path, m->rgx ) == REGEX_MATCH )
			break;

	// fall back to the default
	if( !m )
		m = _rkv->ret_default;

	// copy the buckets
	memcpy( hdr.buckets, m->buckets, m->bct * sizeof( RKBKT ) );

	// fill in the starter
	memcpy( hdr.start.magic, RKV_MAGIC_STR, 4 );
	hdr.start.version  = 1;
	hdr.start.hdr_sz = sizeof( RKHDR );
	hdr.start.buckets  = m->bct;
	hdr.start.filesz   = rkv_get_size( &hdr );

	// try to open the file
	fd = creat( r->fpath, RKV_CREATE_MODE );

	if( fd < 0 )
	{
		err( "Failed to create new file %s -- %s", r->fpath, Err );
		return -2;
	}

	do
	{
		if( fallocate( fd, 0, 0, hdr.start.filesz ) == 0 )
			break;
	
		if( errno == EINTR )
		{
			info( "Interrupted while allocating space (%ul for file %s -- retrying.",
				hdr.start.filesz, r->fpath );
		}
		else
		{
			err( "Failed to allocate disk space (%ul) for file %s -- %s",
				hdr.start.filesz, r->fpath, Err );
			close( fd );
			return -3;
		}
	} while( 1 );

	len = sizeof( RKHDR );

	// create the header
	if( write( fd, &hdr, len ) != len )
	{
		err( "Could not write header to new file %s -- %s", r->fpath, Err );
		close( fd );
		return -4;
	}

	close( fd );
	return 0;
}



// open a file, creating if necessary
int rkv_open( RKFL *r )
{
	struct stat sb;
	int fd, rv, i;
	off_t offs, sz;

	if( stat( r->fpath, &sb ) != 0 )
	{
		switch( errno )
		{
			case ENOENT:
				if( ( rv = rkv_create( r ) ) != 0 )
					return rv;

				if( stat( r->fpath, &sb ) != 0 )
				{
					err( "Could not stat newly-created file %s -- %s",
						r->fpath, Err );
					return -1;
				}
				break;
			default:
				err( "Unable to stat file %s -- %s", r->fpath, Err );
				return -1;
		}
	}

	fd = open( r->fpath, RKV_OPEN_FLAGS );

	if( fd < 0 )
	{
		err( "Unable to open file %s -- %s", r->fpath, Err );
		return -2;
	}

	r->map = mmap( NULL, sb.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0 );

	if( r->map == MAP_FAILED )
	{
		err( "Unable to mmap file %s -- %s", r->fpath, Err );
		close( fd );
		return -3;
	}

	// no longer needed - we are mapped
	close( fd );

	// examine the header
	r->hdr = (RKHDR *) r->map;

	if( memcmp( r->hdr->start.magic, RKV_MAGIC_STR, 4 )
	 || r->hdr->start.version != 1
	 || r->hdr->start.buckets > RKV_FL_MAX_BUCKETS )
	{
		err( "File %s looks invalid.", r->fpath );
		munmap( r->map, sb.st_size );
		return -4;
	}

	// calculate our file offsets for each bucket
	offs = r->hdr->start.hdr_sz;
	sz   = sizeof( PNT );

	for( i = 0; i < r->hdr->start.buckets; ++i )
	{
#ifdef __GNUC__
#if __GNUC_PREREQ(4,8)
#pragma GCC diagnostic ignored "-Wpointer-arith"
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#endif
		// gcc doesn't like pointer arithmetic with voids.  Whatevs.
		r->ptrs[i] = r->map + offs;
#ifdef __GNUC__
#if __GNUC_PREREQ(4,8)
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif
#endif
		offs += r->hdr->buckets[i].count * sz;
		sz = sizeof( PNTA );
	}

	// and we are done - working handle
	return 0;
}



void rkv_close( RKFL *r )
{
	if( !r )
		return;

	if( r->map )
	{
		// sync that, now or soon
		msync( r->map, r->hdr->start.filesz, _rkv->ms_sync );

		munmap( r->map, r->hdr->start.filesz );
		r->map = NULL;
	}

	r->hdr = NULL;
}

void rkv_shutdown( RKFL *r )
{
	rkv_close( r );
	free( r );
}


RKFL *rkv_create_handle( char *path, int plen )
{
	char *last, *p, *q;
	RKFL *r;
	int l;

	if( !path || !*path )
	{
		err( "No path provided to rkv_create_handle." );
		return NULL;
	}

	if( !plen )
		plen = strlen( path );

	if( plen > _rkv->maxpath )
	{
		err( "Path too long - max %d: %s", _rkv->maxpath, path );
		return NULL;
	}

	r = (RKFL *) allocz( sizeof( RKFL ) );

	// base path already has it's / on the end
	l = RKV_EXTENSION_LEN + plen + _rkv->bplen;

	r->fplen  = l;
	r->fpath = (char *) allocz( l + 1 );
	memcpy( r->fpath,                      _rkv->base_path, _rkv->bplen );
	memcpy( r->fpath + _rkv->bplen,        path, plen );
	memcpy( r->fpath + _rkv->bplen + plen, RKV_EXTENSION, RKV_EXTENSION_LEN );

	// don't stomp on the *last* dot, the file extension
	last = r->fpath + _rkv->bplen + plen;

	p = r->fpath;

	// and step over a first . in case file root is a relative path
	if( *p == '.' )
	{
		++p;
		--l;
	}

	// run along the string, stomping on '.' and replacing with /
	while( ( q = memchr( p, '.', l ) ) )
	{
		if( q == last )
			break;

		// keep track of the dir length
		r->dlen = q - r->fpath;

		*q++ = '/';
		l -= q - p;
		p = q;
	}

	// and copy the path
	r->path = str_copy( path, plen );

	return r;
}

