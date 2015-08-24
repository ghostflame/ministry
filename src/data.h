#ifndef MINISTRY_DATA_H
#define MINISTRY_DATA_H

// rounds the structure to 4k
#define PTLIST_SIZE				1021
#define DATA_HASH_SIZE			100003
#define DATA_SUBMIT_INTV		10000000

#define LINE_SEPARATOR			'\n'
#define FIELD_SEPARATOR			' '

enum data_conn_type
{
	DATA_TYPE_DATA = 0,
	DATA_TYPE_STATSD,
	DATA_TYPE_ADDER,
	DATA_TYPE_MAX
};

enum stats_fields
{
	STAT_FIELD_PATH = 0,
	STAT_FIELD_VALUE,
	STAT_FIELD_OP,
	STAT_FIELD_MAX,
};

#define STAT_FIELD_MIN			2

struct points_list
{
	PTLIST			*	next;
	uint32_t			count;
	float				vals[PTLIST_SIZE];
};


struct data_stat_entry
{
	DSTAT			*	next;
	char			*	path;
	PTLIST			*	points;
	PTLIST			*	processing;
	time_t				last;
	uint16_t			len;
	uint16_t			sz;
	uint32_t			id;
	uint32_t			sum;
	uint32_t			total;
	uint16_t			empty;
	uint16_t			unsafe;
};

struct data_add_entry
{
	DADD			*	next;
	char			*	path;
	unsigned long long	total;
	unsigned long long	report;
	uint16_t			len;
	uint16_t			sz;
	uint32_t			sum;
	uint32_t			id;
	uint16_t			empty;
	uint16_t			unsafe;
};



struct data_control
{
	DSTAT			**	stats;
	DADD			**	add;

	unsigned long		spaths;
	unsigned long		apaths;

	int					hsize;

	// in usec
	int					submit_intv;
};






line_fn data_line_data;
line_fn data_line_statsd;
line_fn data_line_adder;

void data_start( NET_TYPE *nt );

void data_init( void );

DATA_CTL *data_config_defaults( void );
int data_config_line( AVP *av );

#endif
