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
	memcpy( b->buf + b->len, t->prefix->buf, t->prefix->len );
	b->len += t->prefix->len;

	// then the path and value
	memcpy( b->buf + b->len, t->path->buf, t->path->len );
	b->len += t->path->len;

	// add the timestamp and newline
	memcpy( b->buf + b->len, ts->buf, ts->len );
	b->len += ts->len;
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
	memcpy( b->buf + b->len, "put ", 4 );
	b->len += 4;

	// copy the prefix
	memcpy( b->buf + b->len, t->prefix->buf, t->prefix->len );
	b->len += t->prefix->len;

	// then just the path
	memcpy( b->buf + b->len, t->path->buf, k );
	b->len += k;

	// now the timestamp, but without the newline
	memcpy( b->buf + b->len, ts->buf, ts->len - 1 );
	b->len += ts->len - 1;

	// now another space
	b->buf[b->len++] = ' ';

	// then the value
	memcpy( b->buf + b->len, space, l );
	b->len += l;

	// then a space, a tag and a newline
	memcpy( b->buf + b->len, " source=ministry\n", 17 );
	b->len += 17;
}






void targets_start( void )
{
	TSET *s;

	// start each target set
	for( s = ctl->tgt->sets; s; s = s->next )
		target_run_list( s->targets, 0 );
}


int targets_init( void )
{
	TGTS_CTL *c = ctl->tgt;
	TGTL *l;
	TSET *s;
	int i;

	// create the target sets
	for( l = ctl->target->lists; l; l = l->next )
	{
		s = (TSET *) allocz( sizeof( TSET ) );
		s->targets = l;
		s->type = (TTYPE *) l->targets->ptr;

		s->next = c->sets;
		c->sets = s;
		c->set_count++;
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
	for( i = 0, s = c->sets; s; s = s->next, i++ )
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

	for( i = 0; i < TGTS_TYPE_MAX; i++, tt++ )
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

