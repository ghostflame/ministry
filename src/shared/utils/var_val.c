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
* utils/utils.c - utility routines                                        *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "shared.h"



#define	VV_NO_CHECKS	0x07
int var_val( char *line, int len, AVP *av, int flags )
{
	char *a, *v, *p, *q, *r, *s;

	if( !line || !av )
		return 1;

	if( ( flags & VV_NO_CHECKS ) == VV_NO_CHECKS )
		flags |= VV_NO_VALS;

	/* blat it */
	memset( av, 0, sizeof( AVP ) );

	av->aptr = av->att;
	av->vptr = av->val;

	p  = line;     // start of line
	r  = line;     // needs init
	s  = p + len;  // end of line
	*s = '\0';     // stamp on the newline

	while( len && isspace( *p ) )
	{
		++p;
		--len;
	}
	if( !( flags & VV_VAL_WHITESPACE )
	 || ( flags & VV_NO_VALS ) )
	{
		while( s > p && isspace( *(s-1) ) )
		{
			*--s = '\0';
			--len;
		}
	}

	/* blank line... */
	if( len < 1 )
	{
		av->status = VV_LINE_BLANK;
		return 0;
	}

	/* comments are successfully read */
	if( *p == '#' )
	{
		av->status = VV_LINE_COMMENT;
		return 0;
	}

	// if we are ignoring values, what we have is an attribute
	if( ( flags & VV_NO_VALS ) )
	{
		a = p;
		av->alen = len;

		// if we're automatically setting vals, set 1
		if( flags & VV_AUTO_VAL )
		{
			v = "1";
			av->vlen = 1;
			// say we found it blank
			av->blank = 1;
		}
		else
			v = "";
	}
	else
	{
		/* search order is =, tab, space */
		q = NULL;
		if( !( flags & VV_NO_EQUAL ) )
			q = memchr( p, '=', len );
		if( !q && !( flags & VV_NO_TAB ) )
			q = memchr( p, '\t', len );
		if( !q && !( flags & VV_NO_SPACE ) )
			q = memchr( p, ' ', len );

		if( !q )
		{
			// are we accepting flag values - no value
			if( flags & VV_AUTO_VAL )
			{
				// then we have an attribute and an auto value
				a        = p;
				av->alen = s - p;
				v        = "1";
				av->vlen = 1;
				av->blank = 1;
				goto vv_finish;
			}

			av->status = VV_LINE_BROKEN;
			return 1;
		}

		// blat the separator
		*q = '\0';
		r  = q + 1;

		if( !( flags & VV_VAL_WHITESPACE ) )
		{
			// start off from here with the value
			while( r < s && isspace( *r ) )
				++r;
		}

		while( q > p && isspace( *(q-1) ) )
			*--q = '\0';

		// OK, let's record those
		a        = p;
		av->alen = q - p;
		v        = r;
		av->vlen = s - r;
	}

	/* got an attribute?  No value might be legal but this ain't */
	if( !av->alen )
	{
		av->status = VV_LINE_NOATT;
		return 1;
	}

vv_finish:

	if( av->alen >= AVP_MAX_ATT )
	{
		av->status = VV_LINE_AOVERLEN;
		return 1;
	}
	if( av->vlen >= AVP_MAX_VAL )
	{
		av->status = VV_LINE_VOVERLEN;
		return 1;
	}

	// were we squashing underscores in attrs?
	if( flags && VV_REMOVE_UDRSCR )
	{
		// we are deprecating key names with _ in them, to enable
		// supporting environment names where we will need to
		// replace _ with . to support the hierarchical keys
		// so warn about any key with _ in it.

		if( memchr( a, '_', av->alen ) )
			warn( "Key %s contains an underscore _, this is deprecated in favour of camelCase.", a );

		// copy without underscores
		for( p = a, q = av->att; *p; p++ )
			if( *p != '_' )
				*q++ = *p;

		// cap it
		*q = '\0';

		// and recalculate the length
		av->alen = q - av->att;
	}
	else
	{
		memcpy( av->att, a, av->alen );
		av->att[av->alen] = '\0';
	}

	memcpy( av->val, v, av->vlen );
	av->val[av->vlen] = '\0';

	// are were lower-casing the attributes?
	if( flags && VV_LOWER_ATT )
		for( p = av->att, q = p + av->alen; p < q; ++p )
			*p = tolower( *p );

	/* looks good */
	av->status = VV_LINE_ATTVAL;
	return 0;
}


