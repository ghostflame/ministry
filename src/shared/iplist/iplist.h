/**************************************************************************
* Copyright 2015 John Denholm                                             *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
*                                                                         *
*                                                                         *
* iplist/iplist.h - defines network ip filtering structures               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_IPLIST_H
#define SHARED_IPLIST_H


#define IPLIST_NEITHER		2
#define IPLIST_POSITIVE		1
#define IPLIST_NEGATIVE		0
#define IPLIST_NOMATCH		-1


#define IPLIST_NET_FREE_TEXT	0x01
#define IPLIST_NET_FREE_NAME	0x02


struct iplist_net
{
	IPNET				*	next;
	IPLIST				*	list;
	char				*	text;
	char				*	name;
	uint32_t				net;
	int8_t					tlen;
	int8_t					flags;
	int8_t					bits;
	int8_t					act;
	void				*	data;	// hang data off this match
};



struct iplist
{
	IPLIST				*	next;
	char				*	name;
	char				*	text;

	IPNET				*	singles;
	IPNET				**	ips;
	IPNET				*	nets;

	uint32_t				count;
	uint32_t				hashsz;

	int8_t					tlen;
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
	pthread_mutex_t			lock;
};


// lookup
IPLIST *iplist_find( char *name );
void iplist_call_data( IPLIST *l, iplist_data_fn *fp, void *arg );
void iplist_explain( IPLIST *l, char *pos, char *neg, char *nei, char *pre );

// testing
int iplist_test_ip_all( IPLIST *l, uint32_t ip, MEMHL *matches );
int iplist_test_ip( IPLIST *l, uint32_t ip, IPNET **p );
int iplist_test_str( IPLIST *l, char *ip, IPNET **p );

// startup
void iplist_init_one( IPLIST *l );
void iplist_init( );

// setup
IPNET *iplist_parse_spec( char *str, int len );
int iplist_append_net( IPLIST *l, IPNET **n );
void iplist_free_net( IPNET *n );
void iplist_free_list( IPLIST *l );
int iplist_add_entry( IPLIST *l, int act, char *str, int len );

void iplist_destroy( IPLIST *l );
IPLIST *iplist_create( char *name, int default_return, int hashsz );

// config
int iplist_set_text( IPLIST *l, char *str, int len );
IPL_CTL *iplist_config_defaults( void );
conf_line_fn iplist_config_line;


#endif
