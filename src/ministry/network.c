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
* net.c - networking setup and config                                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "ministry.h"




NET_TYPE *network_type_defaults( int type )
{
	DTYPE *d = data_type_defns + type;
	NET_TYPE *nt;

	nt              = (NET_TYPE *) mem_perm( sizeof( NET_TYPE ) );
	nt->tcp         = (NET_PORT *) mem_perm( sizeof( NET_PORT ) );
	nt->tcp->ip     = INADDR_ANY;
	nt->tcp->back   = DEFAULT_NET_BACKLOG;
	nt->tcp->port   = d->port;
	nt->tcp->type   = nt;
	nt->flat_parser = d->lf;
	nt->prfx_parser = d->pf;
	nt->handler     = d->af;
	nt->buf_parser  = d->bp;
	nt->udp_bind    = INADDR_ANY;
	nt->threads     = d->thrd;
	nt->pollmax     = TCP_MAX_POLLS;
	nt->tcp_style   = d->styl;
	nt->label       = strdup( d->sock );
	nt->name        = strdup( d->name );
	nt->flags       = NTYPE_ENABLED|NTYPE_TCP_ENABLED|NTYPE_UDP_ENABLED;
	nt->token_bit   = d->tokn;

	// and hook up the dtype
	d->nt = nt;

	net_add_type( nt );
	return nt;
}



NETW_CTL *network_config_defaults( void )
{
	NETW_CTL *netw;

	netw         = (NETW_CTL *) mem_perm( sizeof( NETW_CTL ) );
	netw->stats  = network_type_defaults( DATA_TYPE_STATS );
	netw->adder  = network_type_defaults( DATA_TYPE_ADDER );
	netw->gauge  = network_type_defaults( DATA_TYPE_GAUGE );
	netw->histo  = network_type_defaults( DATA_TYPE_HISTO );
	netw->compat = network_type_defaults( DATA_TYPE_COMPAT );

	return netw;
}


