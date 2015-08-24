#ifndef MINISTRY_MEM_H
#define MINISTRY_MEM_H


#define NEW_PTLIST_BLOCK_SZ				128
#define NEW_DSTAT_BLOCK_SZ				256
#define NEW_DADD_BLOCK_SZ				256

#define MEM_MAX_MB						4096


struct memory_control
{
	HOST		*	hosts;
	PTLIST		*	points;
	DSTAT		*	dstat;
	DADD		*	dadd;

	int				free_hosts;
	int				free_points;
	int				free_dstat;
	int				free_dadd;

	int				mem_hosts;
	int				mem_points;
	int				mem_dstat;
	int				mem_dadd;

	int				max_mb;
};


throw_fn mem_loop;

HOST *mem_new_host( void );
void mem_free_host( HOST **h );

PTLIST *mem_new_point( void );
void mem_free_point( PTLIST **p );
void mem_free_point_list( PTLIST *list );

DSTAT *mem_new_dstat( char *str, int len );
void mem_free_dstat( DSTAT **d );

DADD *mem_new_dadd( char *str, int len );
void mem_free_dadd( DADD **d );

MEM_CTL *mem_config_defaults( void );
int mem_config_line( AVP *av );

#endif
