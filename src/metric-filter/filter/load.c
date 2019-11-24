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
	int i, m, ct, ict, len, mc;
	JSON *jm, *jt, *ji, *je;
	const char *str;
	RGXL *rl = NULL;
	IPLIST *l;
	IPNET *n;
	FILT *fl;

	l = conf->ipl;

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
		for( i = FILTER_MODE_ALL; i < FILTER_MODE_MAX; ++i )
			if( !strcasecmp( str, filter_mode_names[i] ) )
			{
				m = i;
				break;
			}

		if( i == FILTER_MODE_MAX )
		{
			warn( "Filter file %s has invalid mode: %s", path, str );
			return -1;
		}
	}

	if( ( !( ji = json_object_object_get( jo, "ips" ) )
       && !( ji = json_object_object_get( jo, "source" ) ) )
	 || json_object_get_type( ji ) != json_type_array
	 || ( ict = (int) json_object_array_length( ji ) ) == 0 )
	{
		warn( "Filter file %s has no IPs array.", path );
		return -1;
	}

	// do this bit first, tidy up is simpler
	if( m != FILTER_MODE_ALL )
	{
		if( !( jm = json_object_object_get( jo, "matches" ) )
		 || json_object_get_type( jm ) != json_type_array
		 || ( ct = (int) json_object_array_length( jm ) ) == 0 )
		{
			warn( "Filter file %s has no matches array but mode %s.", path,
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
			else
				warn( "Filter file %s: invalid regex: %s", path, (char *) str );
		}

		if( mc == 0 )
		{
			warn( "Filter file %s had no valid regex matches but mode %s.", path,
				filter_mode_names[m] );
			free( rl );
			return -1;
		}
	}

	fl = (FILT *) allocz( sizeof( FILT ) );
	fl->mode = m;
	fl->matches = rl;

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
			n->data = mem_list_create( 0, NULL ); // doesn't need locking
			mc++;
		}

		// add that to this IP net
		mem_list_add_tail( (MEMHL *) n->data, fl );
	}

	if( mc == 0 )
	{
		warn( "Filter file %s had no valid IP addresses/ranges.", path );
		regex_list_destroy( rl );
		free( fl );
	}

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
	char pbuf[2048];
	struct stat sb;
	int i, l, ct;
	JSON *jo;

	info( "Reading filter dir %s", dir );

	files = NULL;
	if( ( ct = scandir( dir, &files, &filter_scan_check, &alphasort ) ) < 0 )
	{
		warn( "Failed to read filter dir %s -- %s", dir, Err );
		return -1;
	}

	fs_treemon_add( conf->watch, dir, 1 );

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

		filter_add_object( conf, jo, pbuf );
		fs_treemon_add( conf->watch, pbuf, 0 );
	}

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
	free( conf );
}


int filter_load( void )
{
	FCONF *conf, *prev;
	char nbuf[64];

	prev = ctl->filt->fconf;

	conf = (FCONF *) allocz( sizeof( FCONF ) );
	conf->watch = fs_treemon_create( "\\.json$", NULL, &filter_on_change, conf );
	conf->active = 1;

	snprintf( nbuf, 64, "filter-list-%ld", ctl->proc->curr_time.tv_sec );
	conf->ipl = iplist_create( nbuf, IPLIST_NEGATIVE, 0 );

	// now we need to 'reload', so create the iplist config
	// for new requests, and tear down the old one.
	// whether we tear down the old one depends on whether
	// we are closing old connections

	filter_scan_dir( ctl->filt->filter_dir, conf );

	// swap them
	ctl->filt->fconf = conf;

	if( prev )
	{
		prev->active = 0;
		filter_unload( prev );
	}

	return 0;
}



int filter_on_change( FTREE *ft, uint32_t mask, char *path, char *desc, void *arg )
{
	FCONF *fc = (FCONF *) arg;
	int ret = 0;

	if( fc->active )
	{
		notice( "Filter change detected - reloading." );

		lock_filters( );
		ret = filter_load( );
		unlock_filters( );

		if( ret < 0 )
			warn( "Failed to reload filters!" );
	}

	return ret;
}


