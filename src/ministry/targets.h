/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* target.h - defines target backend structures and functions              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_TARGETS_H
#define MINISTRY_TARGETS_H

// backend types
enum targets_backends
{
	TGTS_TYPE_UNKNOWN = 0,
	TGTS_TYPE_GRAPHITE,		// set type: graphite (sec)
	TGTS_TYPE_COAL,			// set type: coal (nsec)
	TGTS_TYPE_OPENTSDB,		// set type: opentsdb (msec)
	TGTS_TYPE_MAX
};

// used for config
struct targets_type_data
{
	int						type;
	char				*	name;
	tsf_fn				*	tsfp;
	targets_fn			*	wrfp;
	uint16_t				port;
};




//
// each target set is a group of targets
// that can be sent the same buffers
//
// each set is a list, by shared/target parlance
// so we need to read in a list name
//
// bprintf will write to a buffer for each
// target set
//
struct targets_set
{
	TSET				*	next;
	TGTL				*	targets;
	TTYPE				*	type;
	int						count;
};



struct targets_control
{
	TSET				*	sets;
	TSET				**	setarr;	// flat list pointing to the above
	int						set_count;
};


targets_fn targets_write_graphite;
targets_fn targets_write_opentsdb;

target_cfg_fn targets_set_type;

void targets_start( void );
int targets_init( void );

TGTS_CTL *targets_config_defaults( void );

#endif
