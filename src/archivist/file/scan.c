/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* file/scan.c - discover existing files                                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

int file_scan_filter( const struct dirent *d )
{
	int l;

	// never mind trouble
	if( d->d_name[0] == '.' )
		return 0;

	// recurse into dirs
	if( d->d_type == DT_DIR )
		return 1;

	// but then only regular files
	if( d->d_type != DT_REG )
		return 0;

	// must be long enough to have the file extension on
	if( ( l = strlen( d->d_name ) ) < ( 1 + FILE_EXTENSION_LEN ) )
		return 0;

	// must end in that file extension
	if( strcmp( d->d_name + l - FILE_EXTENSION_LEN, FILE_EXTENSION ) )
		return 0;

	// looking good
	return 1;
}


int file_scan_dir( BUF *fbuf, BUF *pbuf, TEL *prt, int depth )
{
	struct dirent **entries, *d;
	uint32_t fblen, pblen;
	int ect, j, fc;
	char *sl, *dt;
	TEL *t;

	// we need to keep this as we will keep appending to it
	pblen = pbuf->len;
	fblen = fbuf->len;

	entries = NULL;
	fc = 0;

	if( ( ect = scandir( fbuf->buf, &entries, &file_scan_filter, NULL ) ) < 0 )
	{
		err( "File scan failed for dir %s -- %s", pbuf->buf, Err );
		return 0;
	}

	sl = ( depth ) ? "/" : "";
	dt = ( depth ) ? "." : "";

	for( j = 0; j < ect; ++j )
	{
		d = entries[j];

		// add in this name
		strbuf_aprintf( fbuf, "%s%s", sl, d->d_name );
		strbuf_aprintf( pbuf, "%s%s", dt, d->d_name );

		// recurse down into directories
		if( d->d_type == DT_DIR )
		{
			t = tree_insert_node( prt, d->d_name );
			fc += file_scan_dir( fbuf, pbuf, t, depth + 1 );
		}
		else
		{
			// we have to chop the file extension off the end
			strbuf_chopn( pbuf, FILE_EXTENSION_LEN );

			if( tree_insert_leaf( prt, d->d_name, pbuf->buf ) != 0 )
			{
				err( "Could not add leaf %s (%s%s) -- %s",
					pbuf->buf, fbuf->buf, FILE_EXTENSION, Err );
			}
			else
				++fc; 
		}

		strbuf_trunc( fbuf, fblen );
		strbuf_trunc( pbuf, pblen );
	}

	return fc;
}


