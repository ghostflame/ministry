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
* log.h - defines log structures and macros                               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_LOG_LOCAL_H
#define SHARED_LOG_LOCAL_H

#define	DEFAULT_LOG_FILE	"-"   // log to stdout by default
#define DEFAULT_LOG_FAC		LOG_LOCAL4
#define LOG_LINE_MAX		16384


#include "shared.h"


extern LOG_CTL *_logger;


struct log_facility
{
	int					facility;
	char			*	name;
};


struct log_file
{
	char			*	filename;
	char			*	type;
	int8_t				level;
	int8_t				orig_level;

	int					ok_fd;
	int					err_fd;

	int					use_std;
	int					use_syslog;

	int64_t				size;
};


// file.c
int log_file_open( LOGFL *lf );
void log_file_close( LOGFL *lf );
void log_file_reopen( LOGFL *lf );
int log_file_start( LOGFL *lf );
int log_file_set_level( LOGFL *lf, int8_t level, int8_t both );

// conf.c
extern int log_syslog_levels[LOG_LEVEL_MAX];
extern struct log_facility log_facilities[];
extern const char *log_level_strings[LOG_LEVEL_MAX];


void log_set_force_stdout( int set );
int log_get_facility( char *str );

// log.c
int log_open_syslog( void );
int log_close_syslog( void );



#endif
