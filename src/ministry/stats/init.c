/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* stats/control.c - statistics control functions                          *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



void stats_thread_pass( int64_t tval, void *arg )
{
	ST_THR *t = (ST_THR *) arg;
	int i;

	// set up bufs and such
	stats_set_bufs( t, t->conf, tval );

	// do the work
	(*(t->conf->statfn))( t );

	// and report on ourself, except self-stats
	if( t->conf->type != STATS_TYPE_SELF )
		stats_thread_report( t );

	// send any outstanding data
	for( i = 0; i < ctl->tgt->set_count; ++i )
		if( t->bp[i] )
		{
			if( t->bp[i]->len )
			{
				io_buf_post( ctl->tgt->setarr[i]->targets, t->bp[i] );
				t->bp[i] = NULL;
			}
			// not needed?
			// else
			//	mem_free_iobuf( &(t->bp[i]) );
		}
}



void stats_loop( THRD *t )
{
	ST_CFG *cf;
	ST_THR *c;

	c  = (ST_THR *) t->arg;
	cf = c->conf;

	// and then loop round
	loop_control( cf->name, stats_thread_pass, c, cf->period, LOOP_SYNC, cf->offset );
}



void stats_start( ST_CFG *cf )
{
	int i;

	if( cf->enable == 0 )
	{
		notice( "Data reporting for %s is disabled.", cf->name );
		return;
	}

	// throw each of the threads
	for( i = 0; i < cf->threads; ++i )
		thread_throw_named_f( &stats_loop, &(cf->ctls[i]), i, "%s_%d", cf->name, i );

	info( "Started %s data processing loops.", cf->name );
}


void stats_stop( void )
{
	ST_THR *t;

	for( t = ctl->stats->adder->ctls; t; t = t->next )
		pthread_mutex_destroy( &(t->lock) );
}




void stats_init_control( ST_CFG *c, int alloc_data )
{
	ST_MET *sm = ctl->stats->metrics;
	char wkrstrbuf[128], idbuf[12];
	int i, l, j;
	ST_THR *t;
	WORDS w;

	// maybe fall back to default hash size
	if( c->hsize == 0 )
		c->hsize = MEM_HSZ_LARGE;

	debug( "Hash size set to %d for %s", c->hsize, c->name );

	// create the hash structure
	if( alloc_data )
		c->data = (DHASH **) allocz( c->hsize * sizeof( DHASH * ) );

	// convert msec to usec
	c->period *= 1000;
	c->offset *= 1000;
	// offset can't be bigger than period
	c->offset  = c->offset % c->period;

	// make the control structures
	c->ctls = (ST_THR *) allocz( c->threads * sizeof( ST_THR ) );

	w.wc = 4;
	w.wd[0] = "type";
	w.wd[1] = (char *) c->name;
	w.wd[2] = "id";
	w.wd[3] = idbuf;

	for( i = 0; i < c->threads; ++i )
	{
		t = &(c->ctls[i]);

		t->conf   = c;
		t->id     = i;
		t->max    = c->threads;

		// worker path
		snprintf( idbuf, 12, "%lu", t->id );
		l = snprintf( wkrstrbuf, 128, "workers.%s.%s", c->name, idbuf );
		t->wkrstr = str_dup( wkrstrbuf, l );

		// timestamp buffers
		t->ts = (BUF **) allocz( ctl->tgt->set_count * sizeof( BUF * ) );
		for( j = 0; j < ctl->tgt->set_count; ++j )
			t->ts[j] = strbuf( TSBUF_SZ );

		// and a path workspace
		t->path = strbuf( PATH_SZ );

		// and make space for a buffer for each target set we must write to
		t->bp = (IOBUF **) allocz( ctl->tgt->set_count * sizeof( IOBUF * ) );

		// make some floats workspace - we realloc this if needed
		if( c->type == STATS_TYPE_STATS )
		{
			stats_set_workspace( t, MIN_QSORT_THRESHOLD );
			t->counters = (uint32_t *) allocz( 6 * sizeof( uint32_t ) * F8_SORT_HIST_SIZE );
		}

		// create metrics
		t->pm_pts  = pmet_create_gen( sm->pts_count, sm->source, PMET_GEN_IVAL, &(t->points),  NULL, NULL );
		t->pm_high = pmet_create_gen( sm->pts_high,  sm->source, PMET_GEN_IVAL, &(t->highest), NULL, NULL );
		t->pm_tot  = pmet_create_gen( sm->pts_total, sm->source, PMET_GEN_IVAL, &(t->total),   NULL, NULL );
		t->pm_pct  = pmet_create_gen( sm->pct_time,  sm->source, PMET_GEN_DVAL, &(t->percent), NULL, NULL );

		// and apply labels
		pmet_label_apply_item( pmet_label_words( &w ), t->pm_pts  );
		pmet_label_apply_item( pmet_label_words( &w ), t->pm_high );
		pmet_label_apply_item( pmet_label_words( &w ), t->pm_tot  );
		pmet_label_apply_item( pmet_label_words( &w ), t->pm_pct  );

		//pthread_mutex_init( &(t->lock), NULL );

		// and that starts locked
		//lock_stthr( t );
	}
}



void stats_init( void )
{
	struct timespec ts;

	stats_init_control( ctl->stats->stats, 1 );
	stats_init_control( ctl->stats->adder, 1 );
	stats_init_control( ctl->stats->gauge, 1 );

	// let's not always seed from 1, eh?
	clock_gettime( CLOCK_REALTIME, &ts );
	srandom( ts.tv_nsec );

	// we only allow one thread for this, and no data
	ctl->stats->self->threads = 1;
	stats_init_control( ctl->stats->self, 0 );

	// set up the http callback
	http_stats_handler( &stats_self_stats_cb_stats );
}



