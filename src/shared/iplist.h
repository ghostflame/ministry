/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* iplist.h - defines network ip filtering structures                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_IPLIST_H
#define SHARED_IPLIST_H

#define IPLIST_REGEX_OCT	"(25[0-5]|2[0-4][0-9]||1[0-9][0-9]|[1-9][0-9]|[0-9])"
#define IPLIST_REGEX_NET	"^((" IPLIST_REGEX_OCT "\\.){3}" IPLIST_REGEX_OCT ")(/(3[0-2]|[12]?[0-9]))?$"

#define IPLIST_HASHSZ		2003

#define IPLIST_NEITHER		2
#define IPLIST_POSITIVE		1
#define IPLIST_NEGATIVE		0
#define IPLIST_NOMATCH		-1

#define IPLIST_LOCALONLY	"localhost-only"


struct iplist_net
{
	IPNET				*	next;
	IPLIST				*	list;
	char				*	text;
	char				*	name;
	uint32_t				net;
	int16_t					tlen;
	int8_t					bits;
	int8_t					act;
	void				*	data;	// hang data off this match
};



struct iplist
{
	IPLIST				*	next;
	char				*	name;
	char				*	text;

	IPNET				**	ips;
	IPNET				*	nets;

	uint32_t				count;
	uint32_t				hashsz;

	int16_t					tlen;
	int8_t					verbose;
	int8_t					def;
	int8_t					enable;
	int8_t					init_done;
	int8_t					err_dup;
};


struct iplist_control
{
	IPLIST				*	lists;
	int						lcount;
	regex_t				*	netrgx;
};


// lookup
IPLIST *iplist_find( char *name );
void iplist_call_data( IPLIST *l, iplist_data_fn *fp, void *arg );
void iplist_explain( IPLIST *l, char *pos, char *neg, char *nei, char *pre );

// testing
int iplist_test_ip_all( IPLIST *l, uint32_t ip, MEMHG **matches );
int iplist_test_ip( IPLIST *l, uint32_t ip, IPNET **p );
int iplist_test_str( IPLIST *l, char *ip, IPNET **p );

// startup
void iplist_init( );

// setup
IPNET *iplist_parse_spec( char *str, int len );
int iplist_append_net( IPLIST *l, IPNET **n );
void iplist_free_net( IPNET *n );
void iplist_free_list( IPLIST *l );
int iplist_add_entry( IPLIST *l, int act, char *str, int len );

// config
int iplist_set_text( IPLIST *l, char *str, int len );
IPL_CTL *iplist_config_defaults( void );
int iplist_config_line( AVP *av );


#endif
