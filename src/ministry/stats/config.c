/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* stats/config.c - config for stats generation                            *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"



const char *stats_type_names[STATS_TYPE_MAX] =
{
	"stats", "adder", "gauge", "self"
};



STAT_CTL *stats_config_defaults( void )
{
	STAT_CTL *s;

	s = (STAT_CTL *) allocz( sizeof( STAT_CTL ) );

	s->stats          = (ST_CFG *) allocz( sizeof( ST_CFG ) );
	s->stats->threads = DEFAULT_STATS_THREADS;
	s->stats->statfn  = &stats_stats_pass;
	s->stats->period  = DEFAULT_STATS_MSEC;
	s->stats->type    = STATS_TYPE_STATS;
	s->stats->dtype   = DATA_TYPE_STATS;
	s->stats->name    = stats_type_names[STATS_TYPE_STATS];
	s->stats->hsize   = 0;
	s->stats->enable  = 1;
	stats_prefix( s->stats, DEFAULT_STATS_PREFIX );

	s->adder          = (ST_CFG *) allocz( sizeof( ST_CFG ) );
	s->adder->threads = DEFAULT_ADDER_THREADS;
	s->adder->statfn  = &stats_adder_pass;
	s->adder->period  = DEFAULT_STATS_MSEC;
	s->adder->type    = STATS_TYPE_ADDER;
	s->adder->dtype   = DATA_TYPE_ADDER;
	s->adder->name    = stats_type_names[STATS_TYPE_ADDER];
	s->adder->hsize   = 0;
	s->adder->enable  = 1;
	stats_prefix( s->adder, DEFAULT_ADDER_PREFIX );

	s->gauge          = (ST_CFG *) allocz( sizeof( ST_CFG ) );
	s->gauge->threads = DEFAULT_GAUGE_THREADS;
	s->gauge->statfn  = &stats_gauge_pass;
	s->gauge->period  = DEFAULT_STATS_MSEC;
	s->gauge->type    = STATS_TYPE_GAUGE;
	s->gauge->dtype   = DATA_TYPE_GAUGE;
	s->gauge->name    = stats_type_names[STATS_TYPE_GAUGE];
	s->gauge->hsize   = 0;
	s->gauge->enable  = 1;
	stats_prefix( s->gauge, DEFAULT_GAUGE_PREFIX );

	s->self           = (ST_CFG *) allocz( sizeof( ST_CFG ) );
	s->self->threads  = 1;
	s->self->statfn   = &stats_self_stats_pass;
	s->self->period   = DEFAULT_STATS_MSEC;
	s->self->type     = STATS_TYPE_SELF;
	s->self->dtype    = -1;
	s->self->name     = stats_type_names[STATS_TYPE_SELF];
	s->self->enable   = 1;
	stats_prefix( s->self, DEFAULT_SELF_PREFIX );

	// moment checks are off by default
	s->mom            = (ST_MOM *) allocz( sizeof( ST_MOM ) );
	s->mom->min_pts   = DEFAULT_MOM_MIN;
	s->mom->enabled   = 0;
	s->mom->rgx       = regex_list_create( 1 );

	// mode checks are off by default
	s->mode           = (ST_MOM *) allocz( sizeof( ST_MOM ) );
	s->mode->min_pts  = DEFAULT_MODE_MIN;
	s->mode->enabled  = 0;
	s->mode->rgx      = regex_list_create( 1 );

	// predictions are off by default
	s->pred           = (ST_PRED *) allocz( sizeof( ST_PRED ) );
	s->pred->enabled  = 0;
	s->pred->vsize    = DEFAULT_MATHS_PREDICT;
	s->pred->pmax     = DEFAULT_MATHS_PREDICT / 3;
	s->pred->rgx      = regex_list_create( 0 );
	// fixed for now
	s->pred->fp       = &stats_predict_linear;

	// function choice threshold
	s->qsort_thresh   = DEFAULT_QSORT_THRESHOLD;

	return s;
}


int stats_config_line( AVP *av )
{
	char *d, *pm, *lbl, *fmt, thrbuf[64];
	int i, t, l, mid, top;
	ST_THOLD *th;
	STAT_CTL *s;
	ST_CFG *sc;
	int64_t v;
	WORDS wd;

	s = ctl->stats;

	if( !( d = strchr( av->aptr, '.' ) ) )
	{
		if( attIs( "thresholds" ) )
		{
			if( strwords( &wd, av->vptr, av->vlen, ',' ) <= 0 )
			{
				warn( "Invalid thresholds string: %s", av->vptr );
				return -1;
			}
			if( wd.wc > 20 )
			{
				warn( "A maximum of 20 thresholds is allowed." );
				return -1;
			}

			for( i = 0; i < wd.wc; i++ )
			{
				t = atoi( wd.wd[i] );

				// sort out percent from per-mille
				if( ( pm = memchr( wd.wd[i], 'm', wd.len[i] ) ) )
				{
					mid = 500;
					top = 1000;
					lbl = "per-mille";
					fmt = "%s_%03d";
				}
				else
				{
					mid = 50;
					top = 100;
					lbl = "percent";
					fmt = "%s_%02d";
				}

				// sanity check before we go any further
				if( t <= 0 || t == mid || t >= top )
				{
					warn( "A %s threshold value of %s makes no sense: t != %d, 0 < t < %d.",
						lbl, wd.wd[i], mid, top );
					return -1;
				}

				l = snprintf( thrbuf, 64, fmt, ( ( t < mid ) ? "lower" : "upper" ), t );

				// OK, make a struct
				th = (ST_THOLD *) allocz( sizeof( ST_THOLD ) );
				th->val = t;
				th->max = top;
				th->label = str_dup( thrbuf, l );

				th->next = s->thresholds;
				s->thresholds = th;
			}

			debug( "Acquired %d thresholds.", wd.wc );
		}
		else if( attIs( "period" ) )
		{
			av_int( v );
			if( v > 0 )
			{
				s->stats->period = v;
				s->adder->period = v;
				s->gauge->period = v;
				s->self->period  = v;
				debug( "All stats periods set to %d msec.", v );
			}
			else
				warn( "Stats period must be > 0, value %d given.", v );
		}
		else if( attIs( "sortThreshold" ) )
		{
			s->qsort_thresh = av_int( v );

			// don't let it go below 2k, or radix doesn't seem to work right
			if( s->qsort_thresh < MIN_QSORT_THRESHOLD )
			{
				warn( "Sorter thread threshold upped to minimum of %d (from %d).", MIN_QSORT_THRESHOLD, s->qsort_thresh );
				s->qsort_thresh = MIN_QSORT_THRESHOLD;
			}
		}
		else
			return -1;

		return 0;
	}

	d++;

	if( !strncasecmp( av->aptr, "stats.", 6 ) )
		sc = s->stats;
	else if( !strncasecmp( av->aptr, "adder.", 6 ) )
		sc = s->adder;
	// because I'm nice that way (plus, I keep mis-typing it...)
	else if( !strncasecmp( av->aptr, "gauge.", 6 ) || !strncasecmp( av->aptr, "guage.", 6 ) )
		sc = s->gauge;
	else if( !strncasecmp( av->aptr, "self.", 5 ) )
		sc = s->self;
	else if( !strncasecmp( av->aptr, "moments.", 8 ) )
	{
		av->alen -= 8;
		av->aptr += 8;

		if( attIs( "enable" ) )
		{
			s->mom->enabled = config_bool( av );
		}
		else if( attIs( "minimum" ) )
		{
			av_int( s->mom->min_pts );
		}
		else if( attIs( "fallbackMatch" ) )
		{
			t = config_bool( av );
			regex_list_set_fallback( t, s->mom->rgx );
		}
		else if( attIs( "whitelist" ) )
		{
			if( regex_list_add( av->vptr, 0, s->mom->rgx ) )
				return -1;
			debug( "Added moments whitelist regex: %s", av->vptr );
		}
		else if( attIs( "blacklist" ) )
		{
			if( regex_list_add( av->vptr, 1, s->mom->rgx ) )
				return -1;
			debug( "Added moments blacklist regex: %s", av->vptr );
		}
		else
			return -1;

		return 0;
	}
	else if( !strncasecmp( av->aptr, "mode.", 5 ) )
	{
		av->alen -= 5;
		av->aptr += 5;

		if( attIs( "enable" ) )
		{
			s->mode->enabled = config_bool( av );
		}
		else if( attIs( "minimum" ) )
		{
			av_int( s->mode->min_pts );
		}
		else if( attIs( "fallbackMatch" ) )
		{
			t = config_bool( av );
			regex_list_set_fallback( t, s->mode->rgx );
		}
		else if( attIs( "whitelist" ) )
		{
			if( regex_list_add( av->vptr, 0, s->mode->rgx ) )
				return -1;
			debug( "Added mode whitelist regex: %s", av->vptr );
		}
		else if( attIs( "blacklist" ) )
		{
			if( regex_list_add( av->vptr, 1, s->mode->rgx ) )
				return -1;
			debug( "Added mode blacklist regex: %s", av->vptr );
		}
		else
			return -1;

		return 0;
	}
	else if( !strncasecmp( av->aptr, "predict.", 8 ) )
	{
		av->alen -= 8;
		av->aptr += 8;

		if( attIs( "enable" ) )
		{
			s->pred->enabled = config_bool( av );
		}
		else if( attIs( "size" ) )
		{
			if( av_int( v ) == NUM_INVALID )
			{
				err( "Invalid linear regression size '%s'", av->vptr );
				return -1;
			}
			if( v < MIN_MATHS_PREDICT || v > MAX_MATHS_PREDICT )
			{
				err( "Linear regression size must be %d < x <= %d",
					MIN_MATHS_PREDICT, MAX_MATHS_PREDICT );
				return -1;
			}

			s->pred->vsize = (uint16_t) v;
			s->pred->pmax  = s->pred->vsize / 3;
		}
		else if( attIs( "fallbackMatch" ) )
		{
			t = config_bool( av );
			regex_list_set_fallback( t, s->pred->rgx );
		}
		else if( attIs( "whitelist" ) )
		{
			if( regex_list_add( av->vptr, 0, s->pred->rgx ) )
				return -1;
			debug( "Added prediction whitelist regex: %s", av->vptr );
		}
		else if( attIs( "blacklist" ) )
		{
			if( regex_list_add( av->vptr, 1, s->pred->rgx ) )
				return -1;
			debug( "Added prediction blacklist regex: %s", av->vptr );
		}
		else
			return -1;

		return 0;
	}
	else
		return -1;

	av->alen -= d - av->aptr;
	av->aptr  = d;

	if( attIs( "threads" ) )
	{
		av_int( v );
		if( v > 0 )
			sc->threads = v;
		else
			warn( "Stats threads must be > 0, value %d given.", v );
	}
	else if( attIs( "enable" ) )
	{
		if( !( sc->enable = config_bool( av ) ) )
			debug( "Stats type %s disabled.", sc->name );
	}
	else if( attIs( "prefix" ) )
	{
		stats_prefix( sc, av->vptr );
		debug( "%s prefix set to '%s'", sc->name, sc->prefix->buf );
	}
	else if( attIs( "period" ) )
	{
		av_int( v );
		if( v > 0 )
			sc->period = v;
		else
			warn( "Stats period must be > 0, value %d given.", v );
	}
	else if( attIs( "offset" ) || attIs( "delay" ) )
	{
		av_int( v );
		if( v > 0 )
			sc->offset = v;
		else
			warn( "Stats offset must be > 0, value %d given.", v );
	}
	else if( attIs( "size" ) || attIs( "hashSize" ) )
	{
		// 0 means default
		if( !( sc->hsize = hash_size( av->vptr ) ) )
			return -1;
	}
	else
		return -1;

	return 0;
}



