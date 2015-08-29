#ifndef MINISTRY_MEM_H
#define MINISTRY_MEM_H


#define NEW_PTLIST_BLOCK_SZ				128
#define NEW_DHASH_BLOCK_SZ				256
#define MEM_MAX_MB						4096
#define DEFAULT_GC_THRESH				360
#define DEFAULT_MEM_HASHSIZE			100003


struct memory_control
{
	HOST		*	hosts;
	PTLIST		*	points;
	DHASH		*	dhash;
	IOBUF		*	bufs;

	int				free_hosts;
	int				free_points;
	int				free_dhash;
	int				free_bufs;

	int				mem_hosts;
	int				mem_points;
	int				mem_dhash;
	int				mem_bufs;

	int				max_mb;
	int				gc_thresh;
	int				hashsize;
};


throw_fn mem_loop;

HOST *mem_new_host( void );
void mem_free_host( HOST **h );

PTLIST *mem_new_point( void );
void mem_free_point( PTLIST **p );
void mem_free_point_list( PTLIST *list );

DHASH *mem_new_dhash( char *str, int len, int type );
void mem_free_dhash( DHASH **d );
void mem_free_dhash_list( DHASH *list );

IOBUF *mem_new_buf( int sz );
void mem_free_buf( IOBUF **n );
void mem_free_buf_list( IOBUF *list );

MEM_CTL *mem_config_defaults( void );
int mem_config_line( AVP *av );

#endif
