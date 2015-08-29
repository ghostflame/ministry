#ifndef MINISTRY_DATA_H
#define MINISTRY_DATA_H

// rounds the structure to 4k
#define PTLIST_SIZE				1021

#define	DATA_HTYPE_STATS		1
#define	DATA_HTYPE_ADDER		2

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


union data_hash_vals
{
	PTLIST			*	points;
	unsigned long long	total;
};


struct data_hash_entry
{
	DHASH			*	next;
	char			*	path;
	uint16_t			len;
	uint16_t			sz;
	uint32_t			id;
	uint32_t			sum;
	uint16_t			empty;
	uint16_t			type;
	DVAL				in;
	DVAL				proc;
};




line_fn data_line_data;
line_fn data_line_statsd;
line_fn data_line_adder;

void data_start( NET_TYPE *nt );

#endif
