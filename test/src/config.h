/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* config.h - defines main config structure                                * 
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#ifndef MINISTRY_TEST_CONFIG_H
#define MINISTRY_TEST_CONFIG_H

#define DEFAULT_BASE_DIR		"/etc/ministry_test"
#define DEFAULT_CONFIG_FILE		"/etc/ministry_test/ministry_test.conf"


// config flags
#define CONF_FLAG_READ_ENV			0x00000001
#define CONF_FLAG_READ_FILE			0x00000002
#define CONF_FLAG_READ_URL			0x00000004
#define CONF_FLAG_URL_INSEC			0x00000100
#define CONF_FLAG_URL_INC_URL		0x00000200
#define CONF_FLAG_SEC_INC_INSEC		0x00000400
#define CONF_FLAG_TEST_ONLY			0x10000000
#define CONF_FLAG_FILE_OPT			0x20000000

#define setcfFlag( K )				ctl->conf_flags |= CONF_FLAG_##K
#define cutcfFlag( K )				ctl->conf_flags &= ~(CONF_FLAG_##K)
#define chkcfFlag( K )				( ctl->conf_flags & CONF_FLAG_##K )


// used all over - so all the config line fns have an AVP called 'av'
#define attIs( s )      !strcasecmp( av->att, s )
#define valIs( s )      !strcasecmp( av->val, s )


struct config_section
{
	char				*	name;
	conf_line_fn		*	fp;
	int						section;
};

// bring this in from main
extern CSECT config_sections[];

struct config_context
{
	CCTXT				*	next;
	CCTXT				*	children;
	CCTXT				*	parent;
	CSECT				*	section;

	char					source[4096];
	char				**	argv;
	int						lineno;
	int						is_url;
	int						is_ssl;
	int					*	argl;
	int						argc;
};




conf_line_fn config_line;

int config_bool( AVP *av );
int config_read( char *path, WORDS *w );
int config_read_env( char **env );
char *config_relative_path( char *inpath );
void config_choose_section( CCTXT *c, char *section );
void config_create( void );

#endif
