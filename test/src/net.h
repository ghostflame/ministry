/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* net.h - network structures, defaults and declarations                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_TEST_NETWORK_H
#define MINISTRY_TEST_NETWORK_H

#define DEFAULT_STATS_PORT				9125
#define DEFAULT_ADDER_PORT				9225
#define DEFAULT_GAUGE_PORT				9325
#define DEFAULT_COMPAT_PORT				8125
#define DEFAULT_HTTP_PORT				9080
#define DEFAULT_HTTPS_PORT				9443

#define POLL_EVENTS						(POLLIN|POLLPRI|POLLRDNORM|POLLRDBAND|POLLHUP)

#define MIN_NETBUF_SZ					0x10000	// 64k

#define DEFAULT_NET_BACKLOG				32
#define NET_RCV_TMOUT					3		// in sec
#define NET_RECONN_MSEC					3000	// in msec
#define NET_IO_MSEC						500		// in msec

#define DEFAULT_TARGET_HOST				"127.0.0.1"

// ip address regex
#define NET_IP_REGEX_OCT				"(25[0-5]|2[0-4][0-9]||1[0-9][0-9]|[1-9][0-9]|[0-9])"
#define NET_IP_REGEX_STR				"^((" NET_IP_REGEX_OCT "\\.){3}" NET_IP_REGEX_OCT ")(/(3[0-2]|[12]?[0-9]))?$"



// socket for talking to a host
struct net_socket
{
	IOBUF				*	out;
	IOBUF				*	in;

	int						sock;
	int						flags;
	int						bufs;

	struct sockaddr_in		peer;
	char				*	name;
};




NSOCK *net_make_sock( int insz, int outsz, struct sockaddr_in *peer );
//int net_port_sock( PORT_CTL *pc, uint32_t ip, int backlog );
void net_disconnect( int *sock, char *name );

// dns
int net_lookup_host( char *host, struct sockaddr_in *res );

#endif
