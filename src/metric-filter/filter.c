/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* filter.c - filtering functions                                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "metric_filter.h"

const char *filter_mode_names[FILTER_MODE_MAX] =
{
	"all", "allow", "drop"
};



void filter_host_setup( HOST *h )
{

}

void filter_host_end( HOST *h )
{

}



void filter_watcher( THRD *t )
{

}


/*
 * Format:
 * {
 *   "enabled": <bool>,
 *   "type": "<all|allow|drop>",
 *   "ips": [
 *     "a.b.c.d",
 *     "e.f.g.h/i"
 *   ],
 *   "matches": [
 *     "^this.is.matching.",
 *     "!^but.this.is.not."
 *   ]
 * }
 */

int filter_add_filter( IPLIST *l, FFILE *f )
{
	int i, m, ct, ict, len, mc;
	JSON *jm, *jt, *ji, *je;
	const char *str;
	RGXL *rl = NULL;
	IPNET *n;

	// is this one disabled?
	if( ( je = json_object_object_get( f->jo, "enabled" ) )
	 && !json_object_get_boolean( je ) )
	{
		debug( "Filter file %s is disabled.", f->fname );
		return -1;
	}

	// default mode - all
	m = FILTER_MODE_ALL;
	if( !( jt = json_object_object_get( f->jo, "type" ) )
	 && ( str = json_object_get_string( jt ) ) )
	{
		for( i = FILTER_MODE_ALL; i < FILTER_MODE_MAX; ++i )
			if( !strcasecmp( str, filter_mode_names[i] ) )
			{
				m = i;
				break;
			}

		if( i == FILTER_MODE_MAX )
		{
			warn( "Filter file %s has invalid mode: %s", f->fname, str );
			return -1;
		}
	}

	if( !( ji = json_object_object_get( f->jo, "ips" ) )
	 || json_object_get_type( ji ) != json_type_array
	 || ( ict = (int) json_object_array_length( ji ) ) == 0 )
	{
		warn( "Filter file %s has no IPs array.", f->fname );
		return -1;
	}

	// do this bit first, tidy up is simpler
	if( m != FILTER_MODE_ALL )
	{
		if( !( jm = json_object_object_get( f->jo, "matches" ) )
		 || json_object_get_type( jm ) != json_type_array
		 || ( ct = (int) json_object_array_length( jm ) ) == 0 )
		{
			warn( "Filter file %s has no matches array but mode %s.", f->fname,
				filter_mode_names[m] );
			return -1;
		}

		rl = regex_list_create( REGEX_FAIL );

		for( mc = 0, i = 0; i < ct; ++i )
		{
			if( !( je = json_object_array_get_idx( jm, i ) ) )
				continue;

			str = json_object_get_string( je );
			if( regex_list_add( (char *) str, 0, rl ) == 0 )
				mc++;
		}

		if( mc == 0 )
		{
			warn( "Filter file %s had no valid regex matches but mode %s.", f->fname,
				filter_mode_names[m] );
			free( rl );
			return -1;
		}
	}

	for( mc = 0, i = 0; i < ict; ++i )
	{
		if( !( je = json_object_array_get_idx( ji, i ) ) )
			continue;

		str = json_object_get_string( je );
		len = json_object_get_string_len( je );

		if( !( n = iplist_parse_spec( (char *) str, len ) ) )
			continue;

		n->act = IPLIST_POSITIVE;

		if( iplist_append_net( l, &n ) == 0 )
		{
			n->data = mem_list_create( 0 ); // doesn't need locking
			mc++;
		}

		// the check handles duplicates within one file
		if( !mem_list_find( (MEMHL *) n->data, rl ) )
			mem_list_add_tail( (MEMHL *) n->data, rl );
	}

	if( mc == 0 )
	{
		warn( "Filter file %s had no valid IP addresses/ranges.", f->fname );
		regex_list_destroy( rl );
	}

	return 0;
}


void filter_create_map( FCONF *conf )
{
	char buf[64];
	IPLIST *l;
	FFILE *f;

	l = (IPLIST *) allocz( sizeof( IPLIST ) );

	conf->count = 0;

	for( f = conf->files; f; f = f->next )
	{
		if( filter_add_filter( l, f ) == 0 )
			++(conf->count);
	}

	snprintf( buf, 64, "%d @ %ld", conf->count, get_time64( ) );
	l->name = str_copy( buf, 0 );
}

void filter_close_map( FCONF *conf )
{
	mem_free_ffile_list( conf->files );
	free( conf );
}



int filter_scan_check( const struct dirent *d )
{
	int l;

	// only allow directories and files
	if( d->d_type != DT_DIR && d->d_type != DT_REG )
		return 0;

	if( d->d_name[0] == '.' )
		return 0;

	if( ( l = strlen( d->d_name ) ) < 6 )
		return 0;

	if( strcasecmp( d->d_name + l - 5, ".json" ) )
		return 0;

	return 1;
}


FFILE *filter_scan_dir( char *dir )
{
	FFILE *f, *fp, *list = NULL;
	struct dirent **files, *d;
	char pbuf[2048];
	struct stat sb;
	int i, l, ct;
	JSON *jo;

	info( "Reading filter dir %s", dir );

	files = NULL;
	if( ( ct = scandir( dir, &files, &filter_scan_check, &alphasort ) ) < 0 )
	{
		warn( "Failed to read filter dir %s -- %s", dir, Err );
		return NULL;
	}

	for( i = 0; i < ct; ++i )
	{
		d = files[i];
	
		// if you can't give me a path in < 2048 chars, what are you doing?
		l = snprintf( pbuf, 2048, "%s/%s", dir, d->d_name );
		if( l >= 2047 )
		{
			warn( "Path name too long: (%s) %s", d->d_name, pbuf );
			continue;
		}

		// recurse into directories
		if( d->d_type == DT_DIR )
		{
			if( ( f = filter_scan_dir( pbuf ) ) )
			{
				fp = f;
				while( fp->next )
					fp = fp->next;

				fp->next = list;
				list = f;
			}
			continue;
		}

		if( stat( pbuf, &sb ) )
		{
			warn( "Could not stat directory entry '%s' -- %s", d->d_name, Err );
			continue;
		}

		if( !( jo = parse_json_file( NULL, pbuf ) ) )
		{
			warn( "File '%s' could not be parsed correctly, ignoring.", pbuf );
			continue;
		}

		f        = (FFILE *) allocz( sizeof( FFILE ) );
		f->fname = str_copy( pbuf, l );
		f->jo    = jo;
		f->mtime = tsll( sb.st_mtim );
		f->next  = list;
		list     = f;
	}

	// scandir allocs them with malloc, so they need to be freed
	for( i = 0; i < ct; ++i )
		free( files[i] );

	free( files );

	// reverse the list to be in alphasort order
	return (FFILE *) mem_reverse_list( list );
}


int filter_compare_files( FFILE *curr, FFILE *newlist )
{
	FFILE *a, *b;

	/*
	 * Run through the files - each list should be sorted.
	 * If any filenames differ, then we have either a new file or one missing -> reload
	 * If any mtimes differ, then a file has changed -> reload
	 * Only the same files, with the same mtimes, warrant a pass
	 */
	for( a = curr, b = newlist; a && b; a = a->next, b = b->next )
	{
		if( !strcmp( a->fname, b->fname ) )
			return -1;

		if( a->mtime != b->mtime )
			return -1;
	}

	if( a || b )
		return -1;

	return 0;
}


void filter_free_file_list( FFILE *list )
{
	FFILE *f;

	while( list )
	{
		f = list;
		list = f->next;

		free( f->fname );
		json_object_put( f->jo );
		free( f );
	}
}


int filter_load_files( void )
{
	FCONF *conf;
	FFILE *list;

	if( !( list = filter_scan_dir( ctl->filt->filter_dir ) ) )
	{
		notice( "No filters found." );
		return -1;
	}

	if( ctl->filt->fconf && filter_compare_files( ctl->filt->fconf->files, list ) == 0 )
	{
		filter_free_file_list( list );
		return 0;
	}

	conf = (FCONF *) allocz( sizeof( FCONF ) );
	conf->files = list;

	// now we need to 'reload', so create the iplist config
	// for new requests, and tear down the old one.
	// whether we tear down the old one depends on whether
	// we are closing old connections

	filter_create_map( conf );

	// swap them
	conf->next = ctl->filt->fconf;
	ctl->filt->fconf = conf;

	// do we close down the other one?
	if( ctl->filt->close_conn )
	{
		filter_close_map( conf->next );
		conf->next = NULL;
	}

	return 1;
}


int filter_init( void )
{
	ctl->filt->fldlen = strlen( ctl->filt->filter_dir );

	pthread_mutex_init( &(ctl->filt->genlock), NULL );
	if( filter_load_files( ) < 0 )
		return -1;

	thread_throw_named( &filter_watcher, NULL, 0, "filter_watcher" );

	return 0;
}


FLT_CTL *filter_config_defaults( void )
{
	FLT_CTL *f = (FLT_CTL *) allocz( sizeof( FLT_CTL ) );

	f->filter_dir = str_copy( DEFAULT_FILTER_DIR, 0 );
	f->close_conn = 1;

	return f;
}

int filter_config_line( AVP *av )
{
	FLT_CTL *f = ctl->filt;

	if( attIs( "filters" ) || attIs( "filterDir" ) )
	{
		free( f->filter_dir );
		f->filter_dir = str_copy( av->vptr, av->vlen );
		info( "Filter directory: %s", f->filter_dir );
	}
	else if( attIs( "closeConnected" ) )
	{
		f->close_conn = config_bool( av );
	}
	else
		return -1;

	return 0;
}
