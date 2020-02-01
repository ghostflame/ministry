/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* net.h - network structures, defaults and declarations                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef ARCHIVIST_NETWORK_H
#define ARCHIVIST_NETWORK_H

#define DEFAULT_LINE_PORT				3801
#define DEFAULT_BIN_PORT				3811
#define DEFAULT_LINE_TLS_PORT			3802
#define DEFAULT_BIN_TLS_PORT			3812


#define DEFAULT_LINE_PLAIN_THREADS		20
#define DEFAULT_LINE_TLS_THREADS		30
#define DEFAULT_BIN_PLAIN_THREADS		10
#define DEFAULT_BIN_TLS_THREADS			15

enum network_type_id
{
	NETW_TYPE_LINE_PLAIN = 0,
	NETW_TYPE_BIN_PLAIN,
	NETW_TYPE_LINE_TLS,
	NETW_TYPE_BIN_TLS,
	NETW_TYPE_MAX
};

extern const char *network_type_id_names[];

enum network_data_type_id
{
	NETW_DATA_TYPE_LINE = 0,
	NETW_DATA_TYPE_BIN,
	NETW_DATA_TYPE_MAX
};

extern const char *network_data_type_id_names[];


struct network_type_data
{
	int8_t					type;
	int8_t					dtype;
	uint16_t				port;
	int8_t					tls;
	int8_t					style;
	int16_t					threads;
	buf_fn				*	buffp;
	line_fn				*	lnfp;
	char				*	cert_path;
	char				*	key_path;
	char				*	cert;
	char				*	key;
	char				*	key_pass;
	NET_TYPE			*	nt;
};

extern NWTYPE network_type_defns[];

// network record structure
// 2 bytes     - record length
// 2 bytes     - path length (p)
// 2 bytes     - value count (v)
// 2 bytes     - flags
// 8 bytes     - timestamp
// v * 8 bytes - data points
// p bytes     - path
struct network_bin_record_header
{
	uint16_t				rlen;
	uint16_t				plen;
	uint16_t				vcount;
	uint16_t				flags;
	int64_t					ts;
};





struct network_control
{
	NET_TYPE			*	line;
	NET_TYPE			*	sline;
	NET_TYPE			*	bin;
	NET_TYPE			*	sbin;
};



// config
NETW_CTL *network_config_defaults( void );


#endif
