/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* log.h - defines log structures and macros                               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_LOG_H
#define SHARED_LOG_H

#define	DEFAULT_LOG_FILE	"-"   // log to stdout by default
#define DEFAULT_LOG_FAC		LOG_LOCAL4
#define LOG_LINE_MAX		16384


enum log_levels
{
	LOG_LEVEL_FATAL = 0,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_WARN,
	LOG_LEVEL_NOTICE,
	LOG_LEVEL_INFO,
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_MAX
};


struct log_facility
{
	int					facility;
	char			*	name;
};



struct log_control
{
	char			*	filename;
	int8_t				level;

	char			*	identifier;
	int					facility;

	int					ok_fd;
	int					err_fd;

	int8_t				use_std;
	int8_t				use_syslog;
	int8_t				write_level;
	int8_t				force_stdout;
	int8_t				notify_re;
};


// functions from log.c that we want to expose
int log_line( int8_t level, const char *file, const int line, const char *fn, char *fmt, ... );
int log_close( void );
void log_reopen( int sig );
int log_start( void );
int log_set_level( int8_t level );
void log_set_force_stdout( int set );

// config
LOG_CTL *log_config_defaults( void );
conf_line_fn log_config_line;


#define LLFLF( l, ... )         log_line( LOG_LEVEL_##l, __FILE__, __LINE__, __func__, ## __VA_ARGS__ )
#define LLNZN( l, ... )         log_line( LOG_LEVEL_##l,     NULL,        0,     NULL, ## __VA_ARGS__ )

#define fatal( ... )            LLFLF( FATAL,  ## __VA_ARGS__ )
#define err( ... )              LLNZN( ERROR,  ## __VA_ARGS__ )
#define warn( ... )             LLNZN( WARN,   ## __VA_ARGS__ )
#define notice( ... )           LLNZN( NOTICE, ## __VA_ARGS__ )
#define info( ... )             LLNZN( INFO,   ## __VA_ARGS__ )

#ifdef DEBUG
#define debug( ... )            LLFLF( DEBUG,  ## __VA_ARGS__ )
#else
#define debug( ... )            (void) 0
#endif


#endif
