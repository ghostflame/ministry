/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* tcp.c - handles TCP connections                                         *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "carbon_copy.h"


static inline void tcp_hbuf_post( HBUFS *hb, int idx, int replace )
{
	io_buf_post( hb->rule->targets[idx], hb->bufs[idx] );
	hb->bufs[idx] = NULL;

	if( replace )
		hb->bufs[idx] = mem_new_iobuf( IO_BUF_SZ );
}


void tcp_flush_host( HOST *h )
{
	HBUFS *hb;
	int i;

	// send all bufs that have data in
	for( hb = (HBUFS *) h->data; hb; hb = hb->next )
		switch( hb->rule->type )
		{
			case RTYPE_REGEX:
				// single buffer, multi-target
				if( hb->bufs[0] && hb->bufs[0]->len > 0 )
					tcp_hbuf_post( hb, 0, 0 );
				break;

			case RTYPE_HASH:
				// multiple buffers
				for( i = 0; i < hb->bcount; i++ )
					if( hb->bufs[i] && hb->bufs[i]->len > 0 )
						tcp_hbuf_post( hb, i, 0 );
				break;
		}
}




