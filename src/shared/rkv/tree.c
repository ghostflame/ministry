/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* rkv/tree.c - main file-handling functions                               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


uint32_t rkv_tree_get_id( )
{
	uint32_t i;

	rkv_tid_lock( );
	i = ++(_rkv->tid);
	rkv_tid_unlock( );

	return i;
}


// used by file_scan - done without locking
// only inserts directory nodes
TEL *rkv_tree_insert_node( TEL *prt, const char *name )
{
	int len;
	TEL *t;

	len = strlen( name );

	for( t = prt->child; t; t = t->next )
		if( len == t->len && !memcmp( t->name, name, len ) )
			return t;

	t = mem_new_treel( name, len );
	t->parent = prt;

	t->next = prt->child;
	prt->child = t;

	return t;
}

// used by file_scan - done without locking
// only inserts leaf nodes
// name shows up with the file extension on!
int rkv_tree_insert_leaf( TEL *prt, const char *name, const char *path )
{
	int len;
	TEL *t;

	len = strlen( name ) - RKV_EXTENSION_LEN;

	for( t = prt->child; t; t = t->next )
		if( len == t->len && !memcmp( t->name, name, len ) )
		{
			errno = EEXIST;
			return -1;
		}

	t = mem_new_treel( name, len );
	t->parent = prt;

	t->leaf = mem_new_tleaf( );
	t->plen = strlen( path );
	t->path = str_copy( path, t->plen );
	t->leaf->tel = t;

	t->he = string_store_add_with_vals( _rkv->hash, t->path, t->plen, NULL, t );

	// give it an id
	t->id = rkv_tree_get_id( );

	if( !( t->id & 0xffff ) )
		info( "Latest tree element ID is %u", t->id );

	t->next = prt->child;
	prt->child = t;

	return 0;
}


// check again under lock after creating the tree element
// returns 0 if it found a node
// and 1 if it created one
int rkv_tree_insert( char *name, int nlen, char *path, int plen, TEL *prt, TEL **tp )
{
	TEL *t, *r;

	t = mem_new_treel( name, nlen );
	t->parent = prt;

	if( plen )
	{
		t->leaf = mem_new_tleaf( );
		t->plen = plen;
		t->path = str_copy( path, plen );
		t->leaf->tel = t;
	}

	// look under lock, to avoid race conditions
	rkv_tree_lock( prt );

	for( r = prt->child; r; r = r->next )
		if( r->len == nlen && !memcmp( r->name, name, nlen ) )
			break;

	if( !r )
	{
		t->next = prt->child;
		prt->child = t;
	}

	// we are done - this entry exists, so code that found it
	// will not continue
	rkv_tree_unlock( prt );


	if( r )
	{
		*tp = r;

		// this free's the path
		mem_free_treel( &t );

		// did we find something that isn't a leaf on our last word?
		if( (  r->leaf && !plen )
		 || ( !r->leaf &&  plen ) )
			return -1;

		return 0;
	}

	// add it into the hash now that it's ready
	if( t->leaf )
		t->he = string_store_add_with_vals( _rkv->hash, path, plen, NULL, t );

	// give it an id
	t->id = rkv_tree_get_id( );

	if( !( t->id & 0xffff ) )
		info( "Latest tree element ID is %u", t->id );

	return 1;
}


LEAF *rkv_hash_find( char *str, int len )
{
	SSTE *e;

	e = string_store_look( _rkv->hash, str, len, 0 );
	if( e )
		return ((TEL *) (e->ptr))->leaf;

	return NULL;
}


