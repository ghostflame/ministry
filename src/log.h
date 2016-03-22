/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* log.h - defines log structures and macros                               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef MINISTRY_LOG_H
#define MINISTRY_LOG_H

#ifndef Err
#define	Err		strerror( errno )
#endif

#define	DEFAULT_LOG_FILE	"/var/log/ministry/ministry.log"
#define LOG_LINE_MAX		8192


enum log_levels
{
	LOG_LEVEL_FATAL = 0,
	LOG_LEVEL_ERR,
	LOG_LEVEL_WARN,
	LOG_LEVEL_NOTICE,
	LOG_LEVEL_INFO,
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_MAX
};


struct log_control
{
	char			*	filename;
	int					level;
	int					fd;
	int					force_stdout;
	int					notify_re;
};


// functions from log.c that we want to expose
int log_line( int level, const char *file, const int line, const char *fn, char *fmt, ... );
int log_close( void );
void log_reopen( int sig );
int log_start( void );

// config
LOG_CTL *log_config_defaults( void );
int log_config_line( AVP *av );


#define LLFLF( l, f, ... )		log_line( LOG_LEVEL_##l, __FILE__, __LINE__, __FUNCTION__, f, ## __VA_ARGS__ )
#define LLNZN( l, f, ... )		log_line( LOG_LEVEL_##l,     NULL,        0,         NULL, f, ## __VA_ARGS__ )

#define fatal( fmt, ... )		LLFLF( FATAL,  fmt, ## __VA_ARGS__ )
#define err( fmt, ... )			LLNZN( ERR,    fmt, ## __VA_ARGS__ )
#define warn( fmt, ... )		LLNZN( WARN,   fmt, ## __VA_ARGS__ )
#define notice( fmt, ... )		LLNZN( NOTICE, fmt, ## __VA_ARGS__ )
#define info( fmt, ... )		LLNZN( INFO,   fmt, ## __VA_ARGS__ )
#define debug( fmt, ... )		LLFLF( DEBUG,  fmt, ## __VA_ARGS__ )


#endif
