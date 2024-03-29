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
* token.h - defines token handling functions and structures               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_NET_TOKEN_H
#define SHARED_NET_TOKEN_H

#define DEFAULT_TOKEN_LIFETIME		2000	// msec
#define DEFAULT_TOKEN_MASK			0xff	// all types

#define TOKEN_TYPE_STATS			0x01
#define TOKEN_TYPE_ADDER			0x02
#define TOKEN_TYPE_GAUGE			0x04
#define TOKEN_TYPE_HISTO			0x08

#define TOKEN_TYPE_ALL				0xff


struct token_data
{
	TOKEN			*	next;

	NET_TYPE		*	ntype;
	char			*	name;	// assigned, not allocated

	int64_t				expires;
	int64_t				nonce;

	uint32_t			ip;
	int16_t				type;
	int16_t				burned;

	int64_t				id;
};


struct token_info
{
	TOKEN			**	hash;
	IPLIST			*	filter;
	char			*	filter_name;
	uint64_t			hsize;
	int64_t				lifetime;
	int64_t				mask;
	int					enable;
	pthread_mutex_t		lock;
};


void token_burn( TOKEN *t );
TOKEN *token_find( uint32_t ip, int8_t bit, int64_t val );
void token_generate( uint32_t ip, int16_t types, TOKEN **ptrs, int max, int *count );


throw_fn token_loop;

void token_finish( void );
int token_init( void );
TOKENS *token_setup( void );

#endif
