/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* target.c - functions to write to different backend targets              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"


// useful data:
// opentsdb:	http://opentsdb.net/docs/build/html/user_guide/writing.html#telnet



TTYPE targets_type_defns[TGTS_TYPE_MAX] =
{
	{
		.type  = TGTS_TYPE_UNKNOWN,
		.name  = "unknown",
		.port  = 0,
		.tsfp  = NULL,
		.wrfp  = NULL
	},
	{
		.type  = TGTS_TYPE_GRAPHITE,
		.name  = "graphite",
		.port  = 2003,
		.tsfp  = &stats_tsf_sec,
		.wrfp  = &targets_write_graphite
	},
	{
		.type  = TGTS_TYPE_COAL,
		.name  = "coal",
		.port  = 3801,
		.tsfp  = &stats_tsf_nsec,
		.wrfp  = &targets_write_graphite
	},
	{
		.type  = TGTS_TYPE_OPENTSDB,
		.name  = "opentsdb",
		.port  = 0,
		.tsfp  = &stats_tsf_msec,
		.wrfp  = &targets_write_opentsdb
	}
};



// graphite format: <path> <value> <timestamp>\n
void targets_write_graphite( ST_THR *t, BUF *ts, IOBUF *b )
{
	// copy the prefix
	buf_append( b->bf, t->prefix );

	// then the path and value
	buf_append( b->bf, t->path );

	// add the timestamp and newline
	buf_append( b->bf, ts );
}



// tsdb, has to be different
// put <metric> <timestamp> <value> <tagk1=tagv1[ tagk2=tagv2 ...tagkN=tagvN]>
// we don't really have tagging :-(  so fake it
void targets_write_opentsdb( ST_THR *t, BUF *ts, IOBUF *b )
{
	char *space;
	int k, l;

	if( !( space = memrchr( t->path->buf, ' ', t->path->len ) ) )
		return;

	k = space++ - t->path->buf;
	l = t->path->len - k - 1;

	// it starts with a put
	buf_appends( b->bf, "put ", 4 );

	// copy the prefix
	buf_append( b->bf, t->prefix );

	// then just the path
	buf_appends( b->bf, t->path->buf, k );

	// now the timestamp, but without the newline
	buf_append( b->bf, ts );
	strbuf_chop( b->bf );

	// now another space
	buf_addchar( b->bf, ' ' );

	// then the value
	buf_appends( b->bf, space, l );

	// then a space, a tag and a newline
	buf_appends( b->bf, " source=ministry\n", 17 );
}






void targets_start( void )
{
	TSET *s;

	// start each target set
	for( s = ctl->tgt->sets; s; s = s->next )
		target_run_list( s->targets );
}


int targets_init( void )
{
	TGTS_CTL *c = ctl->tgt;
	TGTL *l;
	TSET *s;
	int i;

	// create the target sets
	for( l = target_list_all( ); l; l = l->next )
	{
		s = (TSET *) allocz( sizeof( TSET ) );
		s->targets = l;
		s->type = (TTYPE *) l->targets->ptr;

		s->next = c->sets;
		c->sets = s;
		++(c->set_count);
	}

	if( !c->set_count )
	{
		fatal( "No targets configured." );
		return -1;
	}

	// and make the flat list of set pointers
	c->setarr = (TSET **) allocz( c->set_count * sizeof( TSET * ) );


	// start each target set
	// and make the flat list
	for( i = 0, s = c->sets; s; s = s->next, ++i )
		c->setarr[i] = s;

	return 0;
}




// check the whole list has the same types
int targets_list_check( TGTL *list, TTYPE *tp )
{
	TGT *t;

	if( !list )
		return 0;

	for( t = list->targets; t; t = t->next )
		if( t->ptr != (void *) tp )
			return -1;

	return 0;
}


int targets_set_type( TGT *t, char *type, int len )
{
	TTYPE *tt = targets_type_defns;
	int i;

	for( i = 0; i < TGTS_TYPE_MAX; ++i, ++tt )
	{
		if( strcasecmp( tt->name, type ) )
			continue;

		t->ptr  = tt;
		t->type = tt->type;

		// fill in default port?
		if( t->port == 0 )
			t->port = tt->port;

		return targets_list_check( t->list, tt );
	}

	err( "Unrecognised target type: %s", type );
	return -1;
}



TGTS_CTL *targets_config_defaults( void )
{
	TGTS_CTL *t = (TGTS_CTL *) allocz( sizeof( TGTS_CTL ) );
	return t;
}


