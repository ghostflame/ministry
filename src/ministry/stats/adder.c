/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* stats/adder.c - adder and prediction stats functions                    *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"




// PREDICTOR FUNCTIONS
void stats_predict_linear( ST_THR *t, DHASH *d )
{
	// calculate the values
	maths_predict_linear( d, ctl->stats->pred );

	// report our coefficients and fitness parameter
	// these need to be reported more accurately
	bprintf( t, "%s.lr_a %.10f", d->path, d->predict->a );
	bprintf( t, "%s.lr_b %.10f", d->path, d->predict->b );
	bprintf( t, "%s.fit %f", d->path, d->predict->fit );
}



void stats_predictor( ST_THR *t, DHASH *d )
{
	ST_PRED *sp = ctl->stats->pred;
	PRED *p = d->predict;
	double ts, val;
	uint8_t valid;
	int64_t next;

	val = d->proc.total;
	valid = p->valid;

	// report what we got
	bprintf( t, "%s.input %f", d->path, val );

	// capture the current value
	history_add_point( p->hist, t->tval, val );

	// zero the prediction-only counter if we have a real value
	if( p->pflag )
		p->pflag = 0;
	else
		p->pcount = 0;

	// increment the count
	if( p->valid == 0 )
	{
		p->vcount++;
		if( p->vcount == p->hist->size )
			p->valid = 1;
	}

	// if we've not got enough points yet, we're done
	if( !p->valid )
	{
		debug( "Path %s is not valid yet: %hhu points.", d->path, p->vcount );
		return;
	}

	// were we already valid?
	if( valid )
	{
		// report the diff of the previous prediction against the new value
		bprintf( t, "%s.diff %f", d->path, ( dp_get_v( p->prediction ) - val ) );
	}

	// calculate the next timestamp and put it in
	next = t->tval + ( 1000 * t->conf->period );
	ts = timedbl( next );
	dp_set( p->prediction, ts, 0 );

	// and call the relevant function
	(*(ctl->stats->pred->fp))( t, d );

	// report our newly calculated prediction
	bprintf( t, "%s.predict %f", d->path, dp_get_v( p->prediction ) );

	// have we run the course on pcount?
	if( p->pcount == sp->pmax )
	{
		p->valid = 0;
		p->vcount = 0;
		p->pcount = 0;
	}

	// and keep count
	t->predict++;
}


void stats_adder_pass( ST_THR *t )
{
	SYN_CTL *sc = ctl->synth;
	uint64_t i;
	DHASH *d;

#ifdef DEBUG
	//debug( "[%02d] Adder claim", t->id );
#endif

	st_thr_time( steal );

	// take the data
	for( i = 0; i < t->conf->hsize; i++ )
		if( ( i % t->max ) == t->id )
		{
			for( d = t->conf->data[i]; d && d->valid; d = d->next )
				if( d->in.count > 0 )
				{
					lock_adder( d );

					// copy everything, then zero the in
					d->proc     = d->in;
					d->in.total = 0;
					d->in.count = 0;
					d->do_pass  = 1;

					unlock_adder( d );
				}
				else if( dhash_do_predict( d )
					  && d->predict->valid
					  && d->predict->pcount < ctl->stats->pred->pmax )
				{
					// fill in the number using the predictor
					lock_adder( d );

					// copy it in
					d->proc.total = dp_get_v( d->predict->prediction );
					debug( "Using predicted value: %f", d->proc.total );
					d->proc.count = 1;
					d->do_pass    = 1;
					// mark it as having another prediction used
					d->predict->pcount++;
					d->predict->pflag = 1;

					unlock_adder( d );
				}
				else if( d->empty >= 0 )
					d->empty++;
		}

	st_thr_time( wait );

	// say we are ready
	lock_synth( );
	sc->tready++;
	pthread_cond_signal( &(sc->threads_ready) );
	unlock_synth( );

	// wait for the go
	lock_synth( );
	while( sc->tproceed <= 0 )
	{
		pthread_cond_wait( &(sc->threads_done), &(ctl->locks->synth) );
	}
	// decrement the counter - it hits zero once every thread has done this
	sc->tproceed--;
	unlock_synth( );

#ifdef DEBUG
	//debug( "[%02d] Adder report", t->id );
#endif

	st_thr_time( stats );

	// and report it
	for( i = 0; i < t->conf->hsize; i++ )
		if( ( i % t->max ) == t->id )
			for( d = t->conf->data[i]; d && d->valid; d = d->next )
				if( d->do_pass && d->proc.count > 0 )
				{
					if( d->empty > 0 )
						d->empty = 0;

					if( dhash_do_predict( d ) )
						stats_predictor( t, d );
					else
						bprintf( t, "%s %f", d->path, d->proc.total );

					// keep count and then zero it
					t->points += d->proc.count;
					d->proc.count = 0;

					// and remove the pass marker
					d->do_pass = 0;

					t->active++;
				}

	// keep track of all points
	t->total += t->points;

	// and work out how long that took
	st_thr_time( done );
}


