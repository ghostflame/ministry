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
			if( t->bp[i]->bf->len )
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



void stats_start_one( ST_CFG *cf )
{
	int i;

	if( cf->enable == 0 )
	{
		notice( "Data reporting for %s is disabled.", cf->name );
		return;
	}

	// throw each of the threads
	for( i = 0; i < cf->threads; ++i )
		thread_throw_named_f( &stats_loop, &(cf->ctls[i]), i, "gen_%s_%d", cf->name, i );

	info( "Started %s data processing loops.", cf->name );
}

void stats_stop_one( ST_CFG *cf )
{
	pthread_mutex_destroy( &(cf->statslock) );
}

void stats_start( void )
{
	stats_start_one( ctl->stats->stats );
	stats_start_one( ctl->stats->adder );
	stats_start_one( ctl->stats->gauge );
	stats_start_one( ctl->stats->histo );
	stats_start_one( ctl->stats->self );
}


void stats_stop( void )
{
	ST_THR *t;

	stats_stop_one( ctl->stats->stats );
	stats_stop_one( ctl->stats->adder );
	stats_stop_one( ctl->stats->gauge );
	stats_stop_one( ctl->stats->histo );
	stats_stop_one( ctl->stats->self );

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
		c->data = (DHASH **) mem_perm( c->hsize * sizeof( DHASH * ) );

	// init the stats lock
	pthread_mutex_init( &(c->statslock), &(ctl->proc->mem->mtxa) );

	// convert msec to usec
	c->period *= 1000;
	c->offset *= 1000;
	// offset can't be bigger than period
	c->offset  = c->offset % c->period;

	// make the control structures
	c->ctls = (ST_THR *) mem_perm( c->threads * sizeof( ST_THR ) );

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
		t->wkrstr = str_perm( wkrstrbuf, l );

		// timestamp buffers
		t->ts = (BUF **) mem_perm( ctl->tgt->set_count * sizeof( BUF * ) );
		for( j = 0; j < ctl->tgt->set_count; ++j )
			t->ts[j] = strbuf( TSBUF_SZ );

		// and a path workspace
		t->path = strbuf( PATH_SZ );

		// and make space for a buffer for each target set we must write to
		t->bp = (IOBUF **) mem_perm( ctl->tgt->set_count * sizeof( IOBUF * ) );

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



void stats_init_histograms( void )
{
	ST_HIST *h;

	// do we need to create one?
	if( !ctl->stats->histcf )
	{
		info( "There are no configured histograms - installing a dummy one." );

		h             = (ST_HIST *) mem_perm( sizeof( ST_HIST ) );
		h->rgx        = regex_list_create( 1 );
		h->bounds     = (double *) allocz( sizeof( double ) );
		h->bounds[0]  = 1.0;
		h->brange     = 1;
		h->bcount     = 2;
		regex_list_add( ".", 0, h->rgx );
		h->is_default = 1;
		h->enabled    = 1;

		ctl->stats->histcf = h;
	}

	// look for a default
	for( h = ctl->stats->histcf; h; h = h->next )
		if( h->enabled && h->is_default )
		{
			ctl->stats->histdefl = h;
			break;
		}

	// without a configured default, we choose the 'last' one, that's enabled
	// start at the top of the list
	if( !ctl->stats->histdefl )
	{
		for( h = ctl->stats->histcf; h; h = h->next )
			if( h->enabled )
			{
				ctl->stats->histdefl = h;
				info( "Nominated histogram block '%s' as default.", h->name );
				break;
			}

		// OK, seriously - NO enabled configs?
		// Fine, we are switching the last one one.
		if( !h )
		{
			ctl->stats->histdefl = ctl->stats->histcf;
			ctl->stats->histdefl->enabled = 1;
			warn( "Forcibly enabling histogram block '%s' as the nominated default block.",
				ctl->stats->histdefl->name );
		}
	}

	// reverse the list, back into config order
	mem_reverse_list( &(ctl->stats->histcf) );
}


void stats_init( void )
{
	struct timespec ts;

	stats_init_histograms( );

	stats_init_control( ctl->stats->stats, 1 );
	stats_init_control( ctl->stats->adder, 1 );
	stats_init_control( ctl->stats->gauge, 1 );
	stats_init_control( ctl->stats->histo, 1 );

	// let's not always seed from 1, eh?
	clock_gettime( CLOCK_REALTIME, &ts );
	srandom( ts.tv_nsec );

	// we only allow one thread for this, and no data
	ctl->stats->self->threads = 1;
	stats_init_control( ctl->stats->self, 0 );

	// set up the http callback
	http_stats_handler( &stats_self_stats_cb_stats );
}



