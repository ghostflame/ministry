#ifndef MINISTRY_IO_H
#define MINISTRY_IO_H


#define IO_BUF_SZ				0x40000		// 256k
#define IO_BUF_HWMK				0x3c000		// 240k
#define IO_MAX_WAITING			128			// makes for 128 * 256k = 32M

struct io_buffer
{
	IOBUF		*	next;
	char		*	buf;
	char		*	hwmk;
	int				len;
	int				sz;
};


// r/w
int io_read_data( HOST *h );
int io_read_lines( HOST *h );
int io_write_data( NSOCK *s );

int io_connected( NSOCK *s );
int io_connect( NSOCK *s );

void io_buf_send( IOBUF *buf );

throw_fn io_loop;


#endif
