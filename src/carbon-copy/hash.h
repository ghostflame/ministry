/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* hash.h - defines hashing functions                                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef CARBON_COPY_HASH_H
#define CARBON_COPY_HASH_H

uint32_t hash_fnv1( char *str, int32_t len );
uint32_t hash_fnv1a( char *str, int32_t len );
uint32_t hash_cksum( char *str, int32_t len );

#endif
