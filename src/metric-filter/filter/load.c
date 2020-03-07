/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* filter/load.c - functions to load and parse filters                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


const char *filter_mode_names[FILTER_MODE_MAX] =
{
	"all", "allow", "drop"
};



int filter_get_matches( char *path, JSON *jo, RGXL **rl )
{
	int alen, i, ct;
	JSON *jm, *je;
	const char *s;

	*rl = NULL;

	if( !( jm = json_object_object_get( jo, "matches" ) )
	 || json_object_get_type( jm ) != json_type_array )
	{
		warn( "No matches element found in filter %s.", path );
		return -1;
	}

	alen = json_object_array_length( jm );

	if( alen == 0 )
	{
		warn( "No match strings in array in filter %s.", path );
		return -2;
	}

	*rl = regex_list_create( 0 );

	for( ct = 0, i = 0; i < alen; ++i )
	{
		if( !( je = json_object_array_get_idx( jm, i ) )
		 || json_object_get_type( je ) != json_type_string
		 || !( s = json_object_get_string( je ) ) )
			continue;

		if( regex_list_add( (char *) s, 0, *rl ) == 0 )
			ct++;
		else
			warn( "Filter file %s: invalid regex: %s", path, (char *) s );
	}

	if( ct == 0 )
	{
		warn( "No valid matches in array in filter %s.", path );
		regex_list_destroy( *rl );
		*rl = NULL;
		return -3;
	}

	return 0;
}


int filter_get_sources( char *path, JSON *jo, IPLIST *l, FILT *fl )
{
	int alen, i, ct, len;
	JSON *ji, *je;
	const char *s;
	IPNET *n;

	if( ( !( ji = json_object_object_get( jo, "ips" ) )
       && !( ji = json_object_object_get( jo, "source" ) ) )
	 || json_object_get_type( ji ) != json_type_array )
	{
		warn( "No source IPs defined in filter %s.", path );
		return -1;

	}

	alen = json_object_array_length( ji );

	if( alen == 0 )
	{
		warn( "Empty source IP array in filter %s.", path );
		return -2;
	}

	for( ct = 0, i = 0; i < alen; ++i )
	{
		if( !( je = json_object_array_get_idx( ji, i ) )
		 || json_object_get_type( je ) != json_type_string
		 || !( s = json_object_get_string( je ) )
		 || !( len = json_object_get_string_len( je ) ) )
			continue;

		if( !( n = iplist_parse_spec( (char *) s, len ) ) )
			continue;

		n->act = IPLIST_POSITIVE;

		if( iplist_append_net( l, &n ) == 0 )
		{
			n->data = mem_list_create( 0, NULL ); // doesn't need locking
			ct++;
		}

		// add that to this IP net
		mem_list_add_tail( (MEMHL *) n->data, fl );
	}

	return ct;
}




/*
 * Format:
 * {
 *   "disabled": <bool>,
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

int filter_add_object( FCONF *conf, JSON *jo, char *path )
{
	const char *str;
	RGXL *rl = NULL;
	JSON *jt, *je;
	int m, mc;
	FILT *fl;

	// is this one disabled?
	if( ( je = json_object_object_get( jo, "disabled" ) )
	 && json_object_get_boolean( je ) )
	{
		debug( "Filter file %s is disabled.", path );
		return 0;
	}

	// default mode - all
	m = FILTER_MODE_ALL;
	if( ( jt = json_object_object_get( jo, "type" ) )
	 && ( str = json_object_get_string( jt ) ) )
	{
		m = str_search( str, filter_mode_names, FILTER_MODE_MAX );
		if( m < 0 )
		{
			warn( "Filter file %s has invalid mode: %s", path, str );
			return -1;
		}
	}

	// do this bit first, tidy up is simpler
	if( m != FILTER_MODE_ALL )
	{
		if( filter_get_matches( path, jo, &rl ) != 0 )
			return -1;
	}

	fl = (FILT *) allocz( sizeof( FILT ) );
	fl->mode = m;
	fl->matches = rl;

	mc = filter_get_sources( path, jo, conf->ipl, fl );

	if( mc == 0 )
	{
		warn( "Filter file %s had no valid IP addresses/ranges.", path );
		regex_list_destroy( rl );
		free( fl );
		return -1;
	}

	++(conf->filters);
	return 0;
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


int filter_scan_dir( char *dir, FCONF *conf )
{
	struct dirent **files, *d;
	int i, l, ct;
	char *pbuf;
	JSON *jo;

	info( "Reading filter dir %s", dir );

	files = NULL;
	if( ( ct = scandir( dir, &files, &filter_scan_check, &alphasort ) ) < 0 )
	{
		warn( "Failed to read filter dir %s -- %s", dir, Err );
		return -1;
	}

	fs_treemon_add( conf->watch, dir, 1 );

	pbuf = allocz( 2048 );

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
			filter_scan_dir( pbuf, conf );
			continue;
		}

		if( !( jo = parse_json_file( NULL, pbuf ) ) )
		{
			warn( "File '%s' could not be parsed correctly, ignoring.", pbuf );
			continue;
		}

		if( filter_add_object( conf, jo, pbuf ) == 0 )
			fs_treemon_add( conf->watch, pbuf, 0 );
	}

	for( i = 0; i < ct; ++i )
		free( files[i] );
	
	free( files );
	free( pbuf );

	return 0;
}



void filter_unload( FCONF *conf )
{
	if( !conf )
		return;

	// TODO
	// disconnect all the connections
	// free up the iplists

	fs_treemon_end( conf->watch );
	iplist_free_list( conf->ipl );
	free( conf );
}


int filter_load( void )
{
	FCONF *conf, *prev;
	char nbuf[64];

	prev = ctl->filt->fconf;

	conf = (FCONF *) allocz( sizeof( FCONF ) );
	conf->watch = fs_treemon_create( "\\.json$", NULL, &filter_on_change, conf );

	snprintf( nbuf, 64, "filter-list-%ld", ctl->proc->curr_time.tv_sec );
	conf->ipl = iplist_create( nbuf, IPLIST_NEGATIVE, 0 );
	conf->ipl->enable = 1;

	// now we need to 'reload', so create the iplist config
	// for new requests, and tear down the old one.
	// whether we tear down the old one depends on whether
	// we are closing old connections

	filter_scan_dir( ctl->filt->filter_dir, conf );
	iplist_init_one( conf->ipl );

	info( "Loaded %d filters.", conf->filters );

	// swap them
	conf->active = 1;
	ctl->filt->fconf = conf;

	if( prev )
	{
		prev->active = 0;
		filter_unload( prev );
	}

	return 0;
}



void filter_order_reload( THRD *th )
{
	FCONF *fc = (FCONF *) th->arg;
	int ret;

	// sleep for a delay - merges deltas together
	nanosleep( &(ctl->filt->chg_sleep), NULL );

	if( fc->active )
	{
		notice( "Filter change detected - reloading." );

		lock_filters( );
		ctl->filt->chg_signal = 0;
		ret = filter_load( );
		unlock_filters( );

		if( ret < 0 )
			warn( "Failed to reload filters!" );
	}
}



int filter_on_change( FTREE *ft, uint32_t mask, const char *path, const char *desc, void *arg )
{
	int throw = 0;

	lock_filters( );
	if( ctl->filt->chg_signal == 0 )
	{
		ctl->filt->chg_signal = 1;
		throw = 1;
	}
	unlock_filters( );

	if( throw )
		thread_throw_named( &filter_order_reload, arg, 0, "filter_reload" );

	return 0;
}

