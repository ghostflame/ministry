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
* app.h - startup/shutdown functions                                      *
*                                                                         *
* Updates:                                                                *
**************************************************************************/



#ifndef SHARED_APP_H
#define SHARED_APP_H

int set_signals( void );
int set_limits( void );

int app_init( const char *name, const char *cfgdir, unsigned int flags );
int app_start( int writePid );
void app_ready( void );
void app_finish( int exval );

PROC_CTL *app_control( void );

#endif
