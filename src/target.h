/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* target.h - defines target backend structures and functions              *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_TARGET_H
#define MINISTRY_TARGET_H


#define DEFAULT_MAX_WAITING		128
#define TARGET_MAX_MAX_WAITING	65536


enum target_backends
{
	TGT_TYPE_UNKNOWN = 0,
	TGT_TYPE_GRAPHITE,		// set type: graphite (sec)
	TGT_TYPE_INFLUXDB,		// set type: graphite (sec)
	TGT_TYPE_COAL,			// set type: coal (nsec)
	TGT_TYPE_OPENTSDB,		// set type: opentsdb (msec)
	TGT_TYPE_MAX
};

// protocol to use
enum target_set_types
{
	TGT_STYPE_GRAPHITE = 0,
	TGT_STYPE_COAL,
	TGT_STYPE_OPENTSDB,
	TGT_STYPE_MAX
};

// target set types
struct target_stype_data
{
	int						stype;
	char				*	name;
	tsf_fn				*	tsfp;
	target_fn			*	wrfp;
};

// used for config
struct target_type_data
{
	int						type;
	char				*	name;
	
	uint16_t				port;
	int16_t					st_id;

	TSTYPE				*	stype;
};


// targets and tsets have these
struct target_io_list
{
	IOLIST				*	head;
	IOLIST				*	end;

	int						bufs;

	pthread_mutex_t			lock;
};


//
// each target is one host/port combination
// it has its own buffer list, enabling it to
// keep track of where it is up to
//
struct target_conf
{
	TARGET				*	next;
	TTYPE				*	type;
	TSET				*	set;
	NSOCK				*	sock;
	TGTIO				*	iolist;
	char				*	host;
	char				*	name;

	uint32_t				reconn_ct;
	uint32_t				countdown;
	uint16_t				port;

	// offsets into the current buffer
	int						curr_off;
	int						curr_len;
	int						max;

	pthread_mutex_t			lock;
};


//
// each target set is a group of targets
// that can be sent the same buffers
//
// bprintf will write to a buffer for each
// target set
//
struct target_set
{
	TSET				*	next;
	TSTYPE				*	stype;
	TARGET				*	targets;
	TGTIO				*	iolist;

	int						count;

	pthread_mutex_t			lock;
};




struct target_control
{
	TSET				*	sets;
	TSET				**	setarr;	// flat list pointing to the above
	int						set_count;
	int						tgt_count;
};


target_fn target_write_graphite;
target_fn target_write_coal;
target_fn target_write_opentsdb;


throw_fn target_set_loop;

void target_buf_send( TSET *s, IOBUF *but );
int target_start( void );

TGT_CTL *target_config_defaults( void );
int target_config_line( AVP *av );

#endif
