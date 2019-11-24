/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* memhl.c - memory hanger list functions - doubly linked list             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

// creates a new list
MEMHL *mem_list_filter_new( MEMHL *mhl, void *arg, mhl_callback *cb )
{
	MEMHG *hg;
	MEMHL *fl;

	fl = mem_list_create( mhl->use_lock, mhl->memcb );

	mhl_lock( mhl );

	for( hg = mhl->head; hg; hg = hg->next )
		if( (*cb)( mhl, arg, hg, hg->ptr ) == 0 )
			mem_list_add_tail( fl, hg->ptr );

	mhl_unlock( mhl );

	return fl;
}


int mem_list_filter_self( MEMHL *mhl, void *arg, mhl_callback *cb )
{
	MEMHG *hg, *hg_n;
	int i;

	mhl_lock( mhl );

	for( i = 0, hg = mhl->head; hg; hg = hg_n )
	{
		hg_n = hg->next;

		if( (*cb)( mhl, arg, hg, hg->ptr ) != 0 )
		{
			mem_list_remove( mhl, hg );
			++i;
		}
	}

	mhl_unlock( mhl );

	return i;
}


int mem_list_iterator( MEMHL *mhl, void *arg, mhl_callback *cb )
{
	MEMHG *hg, *hg_n;
	int i, rv;

	mhl_lock( mhl );

	for( i = 0, hg = mhl->head; hg; hg = hg_n )
	{
		hg_n = hg->next;
		rv = (*cb)( mhl, arg, hg, hg->ptr );
		if( rv == 0 )
			++i;
		else if( rv < 0 )
			break;
	}

	mhl_unlock( mhl );

	return i;
}


MEMHG *mem_list_search( MEMHL *mhl, void *arg, mhl_callback *cb )
{
	MEMHG *hg;

	mhl_lock( mhl );

	for( hg = mhl->head; hg; hg = hg->next )
		if( (*cb)( mhl, arg, hg, hg->ptr ) == 0 )
			break;

	mhl_unlock( mhl );

	return hg;
}


MEMHG *mem_list_find( MEMHL *mhl, void *ptr )
{
	MEMHG *hg;

	mhl_lock( mhl );

	for( hg = mhl->head; hg; hg = hg->next )
		if( hg->ptr == ptr )
			break;

	mhl_unlock( mhl );

	return hg;
}


int mem_list_remove( MEMHL *mhl, MEMHG *hg )
{
	if( !hg )
		return -1;

	if( hg->list != mhl )
		return -1;

	mhl_lock( mhl );

	// fix the mhl pointers and surrounding structs
	if( mhl->head == hg )
		mhl->head = hg->next;
	else if( hg->prev )
		hg->prev->next = hg->next;

	if( mhl->tail == hg )
		mhl->tail = hg->prev;
	else if( hg->next )
		hg->next->prev = hg->prev;

	// fix the count
	--(mhl->count);

	mhl_unlock( mhl );

	// have we a cleanup mechanism?
	if( mhl->memcb )
		(*(mhl->memcb))( hg->ptr );

	// free that hanger
	mem_free_hanger( &hg );

	return 0;
}

int mem_list_excise( MEMHL *mhl, void *ptr )
{
	return mem_list_remove( mhl, mem_list_find( mhl, ptr ) );
}


void mem_list_add_tail( MEMHL *mhl, void *ptr )
{
	MEMHG *hg;

	hg = mem_new_hanger( ptr );

	mhl_lock( mhl );

	if( !mhl->tail )
	{
		mhl->head  = hg;
		mhl->tail  = hg;
		mhl->count = 1;
	}
	else
	{
		hg->prev  = mhl->tail;
		mhl->tail->next = hg;
		mhl->tail = hg;
		++(mhl->count);
	}

	mhl_unlock( mhl );
	//info( "Mem list %lu size +t %ld", mhl->id, mhl->count );
}

void mem_list_add_head( MEMHL *mhl, void *ptr )
{
	MEMHG *hg;

	hg = mem_new_hanger( ptr );

	mhl_lock( mhl );

	if( !mhl->head )
	{
		mhl->head  = hg;
		mhl->tail  = hg;
		mhl->count = 1;
	}
	else
	{
		hg->next  = mhl->head;
		mhl->head->prev = hg;
		mhl->head = hg;
		++(mhl->count);
	}

	mhl_unlock( mhl );
	//info( "Mem list %lu size +h %ld", mhl->id, mhl->count );
}

void *mem_list_get_head( MEMHL *mhl )
{
	MEMHG *hg = NULL;
	void *ptr = NULL;

	mhl_lock( mhl );

	if( mhl->head )
	{
		hg = mhl->head;
		mhl->head = hg->next;
		--(mhl->count);

		// are there more?
		if( mhl->head )
			mhl->head->prev = NULL;
		else
			mhl->tail = NULL;

		hg->next = NULL;
	}

	mhl_unlock( mhl );

	//info( "Mem list %lu size -h %ld", mhl->id, mhl->count );

	if( hg )
	{
		ptr = hg->ptr;
		mem_free_hanger( &hg );
	}

	return ptr;
}

void *mem_list_get_tail( MEMHL *mhl )
{
	MEMHG *hg = NULL;
	void *ptr = NULL;

	mhl_lock( mhl );

	if( mhl->tail )
	{
		hg = mhl->tail;
		mhl->tail = hg->prev;
		--(mhl->count);

		// are there more?
		if( mhl->tail )
			mhl->tail->next = NULL;
		else
			mhl->head = NULL;

		hg->prev = NULL;
	}

	mhl_unlock( mhl );

	//info( "Mem list %lu size -t %ld", mhl->id, mhl->count );

	if( hg )
	{
		ptr = hg->ptr;
		mem_free_hanger( &hg );
	}

	return ptr;
}


// freeing all the ptr contents is the calling function's problem
void mem_list_free( MEMHL *mhl )
{
	MEMHG *hg;

	if( !mhl )
		return;

	if( mhl->memcb )
	{
		mhl_lock( mhl );

		for( hg = mhl->head; hg; hg = hg->next )
			(*(mhl->memcb))( hg->ptr );

		mhl_unlock( mhl );
	}

	if( mhl->head )
		mem_free_hanger_list( mhl->head );

	if( mhl->use_lock )
		pthread_mutex_destroy( &(mhl->lock) );

	free( mhl );
}

// prevents locking inside other fns
int mem_list_lock( MEMHL *mhl )
{
	if( !mhl || !mhl->use_lock )
		return -1;

	mhl->act_lock = 0;
	pthread_mutex_lock( &(mhl->lock) );

	return 0;
}

// restores locking inside other fns
int mem_list_unlock( MEMHL *mhl )
{
	if( !mhl || !mhl->use_lock )
		return -1;

	pthread_mutex_unlock( &(mhl->lock) );
	mhl->act_lock = 1;

	return 0;
}

int64_t mem_list_size( MEMHL *mhl )
{
	int64_t ret = 0;

	mhl_lock( mhl );
	ret = mhl->count;
	mhl_unlock( mhl );

	return ret;
}


MEMHL *mem_list_create( int use_lock, mem_free_cb *cb )
{
	MEMHL *mhl = (MEMHL *) allocz( sizeof( MEMHL ) );

	mhl->act_lock = 1;

	if( use_lock )
	{
		mhl->use_lock = 1;
		pthread_mutex_init( &(mhl->lock), NULL );
	}

	if( cb )
		mhl->memcb = cb;

	mhl->id = mem_get_id( );

	return mhl;
}

