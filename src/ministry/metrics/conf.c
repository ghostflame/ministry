/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* metrics/conf.c - functions for metrics configuration                    *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"

MET_CTL *_met = NULL;




// we are expecting path:list-name
int metrics_add_path( METPR *p, char *str, int len )
{
	int l, dv;
	char *cl;
	SSTE *e;

	if( !( cl = memchr( str, ':', len ) ) )
		return -1;

	l = len - ( cl - str );
	len -= cl - str;
	*cl++ = '\0';

	if( !p->paths )
	{
		dv = 1;
		p->paths = string_store_create( 0, "small", &dv, 0 );
	}

	// store the path name only
	if( !( e = string_store_add( p->paths, str, l ) ) )
		return -1;

	// copy the name of the list for now
	// we will have to resolve them later
	e->ptr = str_copy( cl, len );

	return 0;
}


int metrics_add_attr( METAL *m, char *str, int len )
{
	METAT tmp, *a;
	char *cl;

	memset( &tmp, 0, sizeof( METAT ) );

	// were we given an order?
	if( ( cl = memchr( str, ':', len ) ) )
	{
		*cl++ = '\0';
		len -= cl - str;
		tmp.order = strtol( str, NULL, 10 );
		str = cl;
	}
	// if not use order of processing
	else
		tmp.order = m->atct;

	if( !len )
	{
		err( "Empty attribute name." );
		return -1;
	}

	tmp.name = str_perm( str, len );
	tmp.len  = len;

	a = (METAT *) mem_perm( sizeof( METAT ) );
	*a = tmp;

	a->next = m->ats;
	m->ats  = a;
	++(m->atct);

	return 0;
}




MET_CTL *metrics_config_defaults( void )
{
	MET_CTL *m = (MET_CTL *) mem_perm( sizeof( MET_CTL ) );
	METAL *none;

	// we get a free attributes list called "none"
	none = (METAL *) mem_perm( sizeof( METAL ) );
 	none->name = str_perm( "none", 4 );

 	m->alists = none;
 	m->alist_ct = 1;

	_met = m;

	return m;
}


static METAL __metrics_metal_tmp;
static int __metrics_metal_state = 0;

static METPR __metrics_prof_tmp;
static int __metrics_prof_state = 0;

#define _m_inter	{ err( "Metrics attr list and profile config interleaved." ); return -1; }
#define _m_mt_chk	if( __metrics_metal_state ) _m_inter
#define _m_ps_chk	if( __metrics_prof_state )  _m_inter



int metrics_config_line( AVP *av )
{
	METAL *na, *a = &__metrics_metal_tmp;
	METPR *np, *p = &__metrics_prof_tmp;
	METAT *ap;
	METMP *mp;
	int64_t v;

	if( !__metrics_metal_state )
	{
		if( a->name )
			free( a->name );

		while( ( ap = a->ats ) )
		{
			// haemorrage the ap->name memory
			// it's perm space
			a->ats = ap->next;
			free( ap );
		}
		memset( a, 0, sizeof( METAL ) );
	}
	if( !__metrics_prof_state )
	{
		if( p->name )
			free( p->name );

		memset( p, 0, sizeof( METPR ) );
		p->_idctr = 1024;
	}

	if( attIs( "list" ) )
	{
		_m_ps_chk;

		if( a->name )
		{
			warn( "Attribute map '%s' already had a name.", a->name );
			free( a->name );
		}

		a->name = str_copy( av->vptr, av->vlen );
		a->nlen = av->vlen;
		__metrics_metal_state = 1;
	}
	else if( attIs( "attribute" ) || attIs( "attr" ) )
	{
		_m_ps_chk;
		__metrics_metal_state = 1;
		return metrics_add_attr( a, av->vptr, av->vlen );
	}

	// profile values
	else if( attIs( "profile" ) )
	{
		_m_mt_chk;

		if( p->name )
		{
			warn( "Profile '%s' already had a name.", p->name );
			free( p->name );
		}

		p->name = str_copy( av->vptr, av->vlen );
		p->nlen = av->vlen;
		__metrics_prof_state = 1;
	}
	else if( attIs( "default" ) )
	{
		_m_mt_chk;

		p->is_default = config_bool( av );
		__metrics_prof_state = 1;
	}
	else if( attIs( "defaultList" ) )
	{
		_m_mt_chk;

		if( p->default_att )
			free( p->default_att );

		p->default_att = str_copy( av->vptr, av->vlen );
		__metrics_prof_state = 1;
	}
	else if( attIs( "path" ) )
	{
		_m_mt_chk;
		__metrics_prof_state = 1;
		return metrics_add_path( p, av->vptr, av->vlen );
	}
	else if( !strncasecmp( av->aptr, "map.", 4 ) )
	{
		_m_mt_chk;

		av->aptr += 4;
		av->alen -= 4;

		if( attIs( "begin" ) || attIs( "create" ) )
		{
			mp = (METMP *) allocz( sizeof( METMP ) );
			mp->next = p->maps;
			p->maps  = mp;

			mp->id = p->_idctr++;
			mp->enable = 1;
			++(p->mapct);
		}
		else if( attIs( "enable" ) )
		{
			p->maps->enable = config_bool( av );
		}
		else if( attIs( "list" ) )
		{
			if( p->maps->lname )
				free( p->maps->lname );

			p->maps->lname = str_copy( av->vptr, av->vlen );
		}
		else if( attIs( "id" ) )
		{
			av_int( v );
			p->maps->id = (int) v;
		}
		else if( attIs( "last" ) )
		{
			p->maps->last = config_bool( av );
		}
		else if( !strncasecmp( av->aptr, "regex.", 6 ) )
		{
			av->aptr += 6;
			av->alen -= 6;

			if( !p->maps->rlist )
				p->maps->rlist = regex_list_create( 1 );

			if( attIs( "fallbackMatch" ) )
			{
				regex_list_set_fallback( config_bool( av ), p->maps->rlist );
			}
			else if( attIs( "match" ) )
			{
				return regex_list_add( av->vptr, 0, p->maps->rlist );
			}
			else if( attIs( "fail" ) || attIs( "unmatch" ) )
			{
				return regex_list_add( av->vptr, 1, p->maps->rlist );
			}
		}

		__metrics_prof_state = 1;
	}

	else if( attIs( "done" ) )
	{
		// work out which one it is
		if( __metrics_metal_state )
		{
			if( !a->name )
			{
				err( "Cannot proceed with unnamed metrics attribute list." );
				return -1;
			}

			na = (METAL *) allocz( sizeof( METAL ) );
			*na = *a;

			// take everything off the tmp structure, na owns it now
			memset( a, 0, sizeof( METAL ) );

			// sort the attrs
			metrics_sort_attrs( na );

			na->next = _met->alists;
			_met->alists = na;
			++(_met->alist_ct);

			__metrics_metal_state = 0;
		}
		else if( __metrics_prof_state )
		{
			if( !p->name )
			{
				err( "Cannot proceed with unnamed metrics profile." );
				return -1;
			}

			if( !p->maps || !p->paths || !string_store_entries( p->paths ) )
			{
				err( "Metrics profile '%s' will not match anything.  Aborting." );
				return -1;
			}

			// sort the maps into ID order
			mem_sort_list( (void **) p->maps, p->mapct, metrics_cmp_maps );

			for( mp = p->maps; mp; mp = mp->next )
				if( !mp->lname )
				{
					if( p->default_att )
					{
						info( "Metrics profile '%s', map id %d falling back to default attr list '%s'.",
							p->name, mp->id, p->default_att );
						mp->lname = str_copy( p->default_att, 0 );
					}
					else
					{
						err( "Metrics profile '%s', map id %d, has no attribute list target name.", p->name, mp->id );
						return -1;
					}
				}

			np = (METPR *) allocz( sizeof( METPR ) );
			*np = *p;

			np->next = _met->profiles;
			_met->profiles = np;
			++(_met->prof_ct);

			__metrics_prof_state = 0;
		}
		else
		{
			err( "Done called in metrics config for neither an attribute list nor a profile." );
			return -1;
		}
	}
	else
		return -1;

	return 0;
}


#undef _m_inter
#undef _m_mt_chk
#undef _m_ps_chk

