/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* tree.c - functions for manipulating the metric tree                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "archivist.h"

TREE_CTL *_tree = NULL;

uint32_t tree_get_tid( void )
{
	uint32_t i;

	tree_tid_lock( );
	i = ++(_tree->tid);
	tree_tid_unlock( );

	return i;
}


// used by file_scan - done without locking
// only inserts directory nodes
TEL *tree_insert_node( TEL *prt, const char *name )
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
int tree_insert_leaf( TEL *prt, const char *name, const char *path )
{
	int len;
	TEL *t;

	len = strlen( name ) - FILE_EXTENSION_LEN;

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

	t->he = string_store_add_with_vals( _tree->hash, path, t->plen, NULL, t );

	// give it an id
	t->id = tree_get_tid( );

	if( !( t->id & 0xffff ) )
		info( "Latest tree element ID is %u", t->id );

	t->next = prt->child;
	prt->child = t;

	return 0;
}


LEAF *tree_process_line( char *str, int len )
{
	TEL *t, *r, *prt;
	int i, last;
	char *copy;
	WORDS w;
	SSTE *e;

	if( ( e = string_store_look( _tree->hash, str, len, 0 ) ) )
		return ((TEL *) (e->ptr))->leaf;

	prt = _tree->root;
	copy = str_copy( str, len );
	strwords( &w, str, len, '.' );
	last = w.wc - 1;

	for( i = 0; i < w.wc; i++ )
	{
		for( t = prt->child; t; t = t->next )
			if( t->len == w.len[i] && !memcmp( t->name, w.wd[i], t->len ) )
				break;

		if( !t )
		{
			t = mem_new_treel( w.wd[i], w.len[i] );
			t->parent = prt;

			if( i == last )
			{
				t->leaf = mem_new_tleaf( );
				t->path = copy;
				t->plen = len;
				t->leaf->tel = t;
			}

			// look again under lock, to avoid race conditions
			tree_lock( prt );

			for( r = prt->child; r; r = r->next )
				if( r->len == w.len[i] && !memcmp( r->name, w.wd[i], r->len ) )
					break;

			if( !r )
			{
				t->next = prt->child;
				prt->child = t;
			}

			// we are done - this entry exists, so code that found it
			// will not continue
			tree_unlock( prt );

			if( r )
			{
				// this free's copy
				mem_free_treel( &t );

				// did we find something that isn't a leaf on our last word?
				if( r->leaf && i == last )
					return NULL;
			}
			else
			{
				// add it into the hash now that it's ready
				if( t->leaf )
					t->he = string_store_add_with_vals( _tree->hash, copy, len, NULL, t );

				// give it an id
				t->id = tree_get_tid( );

				if( !( t->id & 0xffff ) )
					info( "Latest tree element ID is %u", t->id );
			}

			// however it may want this leaf, but we have not finished making it
		}
		else if( t->leaf )
		{
			// don't need this
			free( copy );

			// is this the last of the path?  if so, all good
		  	if( i == last )
				break;

			// or the path carries on
			// cannot make an internal node at an existing leaf
			return NULL;
		}

		prt = t;
	}

	return t->leaf;
}


int tree_init( void )
{
	_tree->hash = string_store_create( _tree->hashsz, NULL, 0, 1 );
	return 0;
}


TREE_CTL *tree_config_defaults( void )
{
	TREE_CTL *t = (TREE_CTL *) allocz( sizeof( TREE_CTL ) );

	t->root = mem_new_treel( "root", 4 );
	t->hashsz = (int64_t) hash_size( "xlarge" );

	_tree = t;

	return t;
}



int tree_config_line( AVP *av )
{
	int64_t v;

	if( attIs( "hashSize" ) || attIs( "size" ) )
	{
		if( !( v = (int64_t) hash_size( av->vptr ) ) )
			return -1;

		_tree->hashsz = v;
	}
	else
		return -1;

	return 0;
}

