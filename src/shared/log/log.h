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


enum log_levels
{
	LOG_LEVEL_MIN = 0,
	LOG_LEVEL_FATAL = 0,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_WARN,
	LOG_LEVEL_NOTICE,
	LOG_LEVEL_INFO,
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_MAX
};



struct log_control
{
	LOGFL			*	main;
	LOGFL			*	http;
	LOGFL			**	fps;

	char			*	identifier;
	int64_t				counts[LOG_LEVEL_MAX];

	int					facility;

	int8_t				write_level;
	int8_t				force_stdout;
	int8_t				notify_re;
	int8_t				unified;
	int8_t				sl_open;
};


// functions from log.c that we want to expose
int log_line( int fi, int8_t level, const char *file, const int line, const char *fn, char *fmt, ... );
int log_close( void );
void log_reopen( int sig );
int log_start( void );
int log_set_level( int8_t level, int8_t both );
void log_set_force_stdout( int set );

int8_t log_get_level( const char *str );
const char *log_get_level_name( int8_t level );

http_callback log_ctl_setdebug;

// config
LOG_CTL *log_config_defaults( void );
conf_line_fn log_config_line;


#define LLFLF( _f, _l, ... )         log_line( _f, LOG_LEVEL_##_l, __FILE__, __LINE__, __func__, ## __VA_ARGS__ )
#define LLNZN( _f, _l, ... )         log_line( _f, LOG_LEVEL_##_l,     NULL,        0,     NULL, ## __VA_ARGS__ )

#define fatal( ... )            LLFLF( 0, FATAL,  ## __VA_ARGS__ )
#define err( ... )              LLNZN( 0, ERROR,  ## __VA_ARGS__ )
#define warn( ... )             LLNZN( 0, WARN,   ## __VA_ARGS__ )
#define notice( ... )           LLNZN( 0, NOTICE, ## __VA_ARGS__ )
#define info( ... )             LLNZN( 0, INFO,   ## __VA_ARGS__ )
#define debug( ... )            LLFLF( 0, DEBUG,  ## __VA_ARGS__ )


#define hfatal( ... )           LLFLF( 1, FATAL,  ## __VA_ARGS__ )
#define herr( ... )             LLNZN( 1, ERROR,  ## __VA_ARGS__ )
#define hwarn( ... )            LLNZN( 1, WARN,   ## __VA_ARGS__ )
#define hnotice( ... )          LLNZN( 1, NOTICE, ## __VA_ARGS__ )
#define hinfo( ... )            LLNZN( 1, INFO,   ## __VA_ARGS__ )
#define hdebug( ... )           LLFLF( 1, DEBUG,  ## __VA_ARGS__ )

#endif
