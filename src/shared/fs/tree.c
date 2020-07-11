/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* fs/tree.c - scan and monitor a file tree for changes                    *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"


#define INOT_MASK		(\
IN_CREATE|\
IN_DELETE|\
IN_DELETE_SELF|\
IN_MODIFY|\
IN_MOVE_SELF|\
IN_MOVE|\
IN_DONT_FOLLOW|\
IN_EXCL_UNLINK)


const char *fs_treemon_event_names[32] =
{
	"access",			// 0x00000001
	"modify",			// 0x00000002
	"attribute",		// 0x00000004
	"close-write",		// 0x00000008
	"close-nowrite",	// 0x00000010
	"open",				// 0x00000020
	"moved-from",		// 0x00000040
	"moved-to",			// 0x00000080
	"create",			// 0x00000100
	"delete",			// 0x00000200
	"delete-self",		// 0x00000400
	"move-self",		// 0x00000800
	"unused-12",		// 0x00001000
	"unmount",			// 0x00002000
	"queue-overflow",	// 0x00004000
	"ignored",			// 0x00008000
	"unused-16",		// 0x00010000
	"unused-17",		// 0x00020000
	"unused-18",		// 0x00040000
	"unused-19",		// 0x00080000
	"unused-20",		// 0x00100000
	"unused-21",		// 0x00200000
	"unused-22",		// 0x00400000
	"unused-23",		// 0x00800000
	// mask flags that determine behaviour
	"onlydir",			// 0x01000000
	"dont-follow",		// 0x02000000
	"exclude-unlinked",	// 0x04000000
	"unused-27",		// 0x08000000
	"mask-create",		// 0x10000000
	"mask-add",			// 0x20000000
	"is-dir",			// 0x40000000
	"one-shot"			// 0x80000000
};


void fs_treemom_free_watch( void *ptr )
{
	FWTCH *f = (FWTCH *) ptr;

	free( f->path );
	free( f );
}

int fs_treemon_filter_out_path( MEMHL *mhl, void *arg, MEMHG *hgr, void *ptr )
{
	FWTCH *fwa = (FWTCH *) arg;
	FWTCH *fwp = (FWTCH *) ptr;

	return strcmp( fwp->path, fwa->path );
}

int fs_treemon_filter_out_wd( MEMHL *mhl, void *arg, MEMHG *hgr, void *ptr )
{
	FWTCH *fwa = (FWTCH *) arg;
	FWTCH *fwp = (FWTCH *) ptr;

	return ( fwp->wd - fwa->wd );
}


int fs_treemon_rm( FTREE *ft, const char *path )
{
	FWTCH fw, *w;
	MEMHG *hg;

	fw.path = (char *) path;
	fw.tree = ft;
	fw.wd   = -1;
	fw.next = NULL;

	if( ( hg = mem_list_search( ft->watches, &fw, &fs_treemon_filter_out_path ) ) )
	{
		w = (FWTCH *) hg->ptr;
		inotify_rm_watch( ft->inFd, (int) w->wd );
		mem_list_excise( ft->watches, hg );
		return 0;
	}

	return -1;
}

int fs_treemon_add( FTREE *ft, const char *path, int is_dir )
{
	FWTCH *fw;
	int wd;

	if( !ft || !path || !*path )
	{
		err( "No file tree or no path." );
		return -1;
	}

	wd = (int32_t) inotify_add_watch( ft->inFd, path, INOT_MASK );

	if( wd < 0 )
	{
		warn( "Failed to add watch for %s -- %s", path, Err );
		return -2;
	}

	fw = (FWTCH *) allocz( sizeof( FWTCH ) );
	fw->path   = str_copy( path, 0 );
	fw->tree   = ft;
	fw->is_dir = is_dir;
	fw->wd     = wd;

	mem_list_add_tail( ft->watches, fw );

	debug( "Added watch (%d) for events %08x for %s.", fw->wd, INOT_MASK, fw->path );
	return 0;
}


int fs_treemon_event_string( uint32_t mask, char *buf, int sz )
{
	int j, len = 0;

	buf[0] = '\0';

	// don't read the control bits, so stay below 24
	for( j = 0; j < 24; ++j )
	{
		if( mask & 0x1 )
			len += snprintf( buf + len, sz - len, "%s%s", ( len ) ? "," : "", fs_treemon_event_names[j] );

		mask >>= 1;

		if( len >= ( sz - 1 ) )
			break;
	}

	return len;
}


void fs_treemon_hdlr( FTREE *ft, struct inotify_event *ev )
{
	char descbuf[512];
	FWTCH tmp, *fw;
	char *path;
	MEMHG *hg;

	tmp.path = NULL;
	tmp.wd   = ev->wd;
	tmp.tree = ft;
	tmp.next = NULL;

	fs_treemon_event_string( ev->mask, descbuf, 512 );

	debug( "Got event for wd=%d, cookie=%d, mask=0x%08x (%s), path=%s",
		ev->wd, ev->cookie, ev->mask, descbuf, ev->name );

	if( ( hg = mem_list_search( ft->watches, &tmp, &fs_treemon_filter_out_wd ) ) )
	{
		fw = (FWTCH *) hg->ptr;
		path = fw->path;
	}
	else
		path = ev->name;

	// and call the callback
	(*(ft->cb))( ft, ev->mask, path, descbuf, ft->arg );
}




void fs_treemon_loop( THRD *thrd )
{
	char *ptr, *end, buf[IO_BUF_SMALL] __attribute__ ((aligned(8)));
	struct inotify_event *ev;
	FTREE *ft;
	int len;

	ft = (FTREE *) thrd->arg;

	while( RUNNING( ) )
	{
		len = read( ft->inFd, buf, IO_BUF_SMALL );
		if( !len )
		{
			err( "Read() from inotify returned 0." );
			break;
		}

		if( len < 0 )
		{
			if( errno == EINTR )
				continue;

			err( "Failed to read() from inotify -- %s", Err );
			break;
		}

		info( "Read %d bytes from inotify.", len );
		end = buf + len;

		for( ptr = buf; ptr < end; )
		{
			ev = (struct inotify_event *) ptr;
			ptr += sizeof( struct inotify_event ) + ev->len;

			fs_treemon_hdlr( ft, ev );
		}
	}

	debug( "Inotify watch finished." );
}



void fs_treemon_start( FTREE *ft )
{
	if( ft->started )
		return;

	ft->started = 1;
	thread_throw_named_f( &fs_treemon_loop, ft, 0, "fstree_%d", ft->inFd );
}

void fs_treemon_start_all( void )
{
	FTREE *ft;

	for( ft = _proc->watches; ft; ft = ft->next )
		fs_treemon_start( ft );
}


void fs_treemon_end( FTREE *ft )
{
	FTREE *prv, *nxt, *f;

	config_lock( );

	// chop it out of the linked list
	for( prv = NULL, f = _proc->watches; f; f = nxt )
	{
		nxt = f->next;

		if( f == ft )
		{
			if( prv )
				prv->next = nxt;
			else
				_proc->watches = nxt;
		}
		else
			prv = f;
	}

	config_unlock( );

	// this apparently tidies up everything
	close( ft->inFd );

	if( ft->fpattern )
	{
		regfree( &(ft->fmatch) );
		free( ft->fpattern );
	}

	if( ft->dpattern )
	{
		regfree( &(ft->dmatch ) );
		free( ft->dpattern );
	}

	mem_list_free( ft->watches );

	free( ft );
}


FTREE *fs_treemon_create( const char *file_pattern, const char *dir_pattern, ftree_callback *cb, void *arg )
{
	char errbuf[1024];
	FTREE *ft;
	int rc;

	if( !cb )
	{
		err( "Creating a tree monitor requires a callback." );
		return NULL;
	}

	ft = (FTREE *) allocz( sizeof( FTREE ) );
	ft->cb  = cb;
	ft->arg = arg;

	if( file_pattern )
	{
		rc = regcomp( &(ft->fmatch), file_pattern, REG_EXTENDED|REG_ICASE );
		if( rc != 0 )
		{
			regerror( rc, &(ft->fmatch), errbuf, 1024 );
			err( "Unable to compile file pattern '%s' -- %s", file_pattern, errbuf );
			free( ft );
			return NULL;
		}

		ft->fpattern = str_copy( file_pattern, 0 );
	}

	if( dir_pattern )
	{
		rc = regcomp( &(ft->dmatch), dir_pattern, REG_EXTENDED|REG_ICASE );
		if( rc != 0 )
		{
			regerror( rc, &(ft->dmatch), errbuf, 1024 );
			err( "Unable to compile dir pattern '%s' -- %s", dir_pattern, errbuf );
			if( ft->fpattern )
			{
				free( ft->fpattern );
				regfree( &(ft->fmatch ) );
			}
			free( ft );
			return NULL;
		}

		ft->dpattern = str_copy( dir_pattern, 0 );
	}

	ft->watches = mem_list_create( 0, &fs_treemom_free_watch );
	ft->inFd = inotify_init1( 0 );

	config_lock( );

	ft->next = _proc->watches;
	_proc->watches = ft;

	config_unlock( );

	return ft;
}

