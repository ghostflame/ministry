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
* hash.h - defines hashing functions                                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef CARBON_COPY_HASH_H
#define CARBON_COPY_HASH_H

//uint32_t hash_fnv1( char *str, int32_t len );
//uint32_t hash_fnv1a( char *str, int32_t len );
//uint32_t hash_cksum( char *str, int32_t len );

hash_fn hash_fnv1;
hash_fn hash_fnv1a;
hash_fn hash_cksum;

#endif
