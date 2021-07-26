/**************************************************************************
* Copyright 2015 John Denholm                                             *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
*                                                                         *
*                                                                         *
* rkv/scan.c - discover existing files                                    *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

int rkv_scan_filter( const struct dirent *d )
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
	if( ( l = strlen( d->d_name ) ) < ( 1 + RKV_EXTENSION_LEN ) )
		return 0;

	// must end in that file extension
	if( strcmp( d->d_name + l - RKV_EXTENSION_LEN, RKV_EXTENSION ) )
		return 0;

	// looking good
	return 1;
}


int rkv_scan_dir( BUF *fbuf, BUF *pbuf, int depth, TEL *prt )
{
	struct dirent **entries, *d;
	uint32_t fblen, pblen;
	int ect, j, fc;
	char *sl, *dt;

	// we need to keep this as we will keep appending to it
	pblen = pbuf->len;
	fblen = fbuf->len;

	entries = NULL;
	fc = 0;

	if( ( ect = scandir( fbuf->buf, &entries, &rkv_scan_filter, NULL ) ) < 0 )
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
			fc += rkv_scan_dir( fbuf, pbuf, depth + 1, rkv_tree_insert_node( prt, d->d_name ) );
		}
		else
		{
			// we have to chop the file extension off the end
			strbuf_chopn( pbuf, RKV_EXTENSION_LEN );

			if( rkv_tree_insert_leaf( prt, d->d_name, pbuf->buf ) != 0 )
			{
				err( "Could not add leaf %s (%s%s) -- %s",
					pbuf->buf, fbuf->buf, RKV_EXTENSION, Err );
			}
			else
				++fc; 
		}

		strbuf_trunc( fbuf, fblen );
		strbuf_trunc( pbuf, pblen );
		free( d );
	}

	free( entries );
	return fc;
}


