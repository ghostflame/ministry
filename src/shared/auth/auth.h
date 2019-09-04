/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* auth/auth.h - defines auth structures and functions                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_AUTH_H
#define SHARED_AUTH_H



struct auth_mechanism
{
	AUTHM					*	next;

	char					*	name;

	int32_t						nlen;
}



// stores users/groups
struct auth_group
{
	AUTHG					*	next;
	SSTR					**	store;
	AUTHM					*	mech;

	char					*	name;
	char					*	mname;

	int32_t						nlen;
	int32_t						mlen;
};



struct auth_control
{
	AUTHG					*	blocks;
	AUTHM					*	mechs;

	int32_t						flags;
};

#endif
