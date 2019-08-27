
struct host_buffers
{
	HBUFS				*	next;
	RELAY				*	rule;
	IOBUF				**	bufs;
	int						bcount;
};

