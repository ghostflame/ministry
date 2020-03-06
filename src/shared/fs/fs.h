/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* fs/fs.h - routines and structures for file systems                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef SHARED_FS_H
#define SHARED_FS_H


struct file_watch
{
	FWTCH				*	next;
	FTREE				*	tree;
	char				*	path;
	int32_t					wd;
	int8_t					is_dir;
};



struct file_tree
{
	FTREE				*	next;
	ftree_callback		*	cb;
	MEMHL				*	watches;

	void				*	arg;		// passed to callback

	char				*	fpattern;
	char				*	dpattern;

	regex_t					fmatch;
	regex_t					dmatch;

	int						inFd;
};







// FUNCTIONS


throw_fn fs_treemon_loop;


// make a directory
int fs_mkdir_recursive( const char *path );

// handle our pidfile
int fs_pidfile_mkdir( const char *path );
void fs_pidfile_write( void );
void fs_pidfile_remove( void );

// file-tree monitor
FTREE *fs_treemon_create( const char *file_pattern, const char *dir_pattern, ftree_callback *cb, void *arg );
int fs_treemon_add( FTREE *ft, const char *path, int is_dir );
int fs_treemon_rm( FTREE *ft, const char *path );
int fs_treemon_event_string( uint32_t mask, char *buf, int sz );
void fs_treemon_end( FTREE *ft );
void fs_treemon_start( FTREE *ft );
void fs_treemon_start_all( void );

#endif
