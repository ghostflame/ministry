#ifndef MINISTRY_NETWORK_H
#define MINISTRY_NETWORK_H

#define DEFAULT_DATA_PORT				9125
#define DEFAULT_STATSD_PORT				8125
#define DEFAULT_ADDER_PORT				9225


#define POLL_EVENTS						(POLLIN|POLLPRI|POLLRDNORM|POLLRDBAND|POLLHUP)

#define MIN_NETBUF_SZ					0x10000	// 64k

#define HOST_CLOSE						0x01
#define HOST_NEW						0x02

#define DEFAULT_NET_BACKLOG				32
#define NET_DEAD_CONN_TIMER				3600	// 1 hr
#define NET_RCV_TMOUT					3		// in sec

#define NTYPE_ENABLED					0x0001
#define NTYPE_TCP_ENABLED				0x0002
#define NTYPE_UDP_ENABLED				0x0004



struct net_buffer
{
	unsigned char		*	buf;
	unsigned char		*	hwmk;
	char				*	ptr;
	int						len;
	int						sz;
};


// socket for talking to a host
struct net_socket
{
	NBUF				*	out;
	NBUF				*	in;
	// this one has no buffer allocation
	NBUF				*	keep;

	int						sock;
	int						flags;

	struct sockaddr_in	*	peer;
	char				*	name;
};



struct host_data
{
	HOST				*	next;
	NSOCK				*	net;

	WORDS				*	all;		// each line
	WORDS				*	val;		// per line

	NET_TYPE			*	type;

	struct sockaddr_in		peer;

	time_t					started;	// accept time
	time_t					last;		// last communication

	unsigned long			points;		// counter
	unsigned long			invalid;	// counter

	int						flags;
};



struct net_in_port
{
	int						sock;
	unsigned long long		counter;

	uint16_t				port;
	uint16_t				back;
	uint32_t				ip;

	NET_TYPE			*	type;
};


struct net_type
{
	NET_PORT			*	tcp;
	NET_PORT			**	udp;
	line_fn				*	handler;
	char				*	label;

	uint16_t				flags;
	uint16_t				udp_count;
	uint32_t				udp_bind;
};



struct network_control
{
	time_t					dead_time;
	unsigned int			rcv_tmout;

	NET_TYPE			*	data;
	NET_TYPE			*	statsd;
	NET_TYPE			*	adder;
};




// thread control
void *net_watched_socket( void *arg );

// client connections
HOST *net_get_host( int sock, NET_TYPE *type );
void net_close_host( HOST *h );

NSOCK *net_make_sock( int insz, int outsz, char *name, struct sockaddr_in *peer );
//int net_port_sock( PORT_CTL *pc, uint32_t ip, int backlog );
void net_disconnect( int *sock, char *name );
int net_connect( NSOCK *s );

// r/w
int net_read_data( NSOCK *s );
int net_read_lines( HOST *h );
int net_write_data( NSOCK *s );

// init/shutdown
int net_start( void );
void net_stop( void );

// config
NET_CTL *net_config_defaults( void );
int net_config_line( AVP *av );

#endif
