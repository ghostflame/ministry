#include "ministry.h"



CCTXT *ctxt_top = NULL;
CCTXT *context  = NULL;

#define CFG_VV_FLAGS	(VV_AUTO_VAL|VV_LOWER_ATT)

// read a file until we find a config line
char __cfg_read_line[1024];
int config_get_line( FILE *f, AVP *av )
{
	char *n, *e;

	while( 1 )
	{
		// end things on a problem
		if( !f || feof( f ) || ferror( f ) )
			break;
		if( !fgets( __cfg_read_line, 1024, f ) )
			break;

		// where are we?
		context->lineno++;

		// find the line end
		if( !( n = memchr( __cfg_read_line, '\n', 1023 ) ) )
			continue;
		*n = '\0';

		// spot sections
		if( __cfg_read_line[0] == '[' )
		{
			if( !( e = memchr( __cfg_read_line, ']', n - __cfg_read_line ) ) )
			{
				warn( "Bad section heading in file '%s': %s",
					context->file, __cfg_read_line );
				return -1;
			}
			*e = '\0';

			if( ( e - __cfg_read_line ) > 511 )
			{
				warn( "Over-long section heading in file '%s': %s",
					context->file, __cfg_read_line );
				return -1;
			}

			// grab that
			snprintf( context->section, 512, "%s", __cfg_read_line + 1 );

			// spotted a section
			return 1;
		}

		// returns 0 if it can cope
		if( var_val( __cfg_read_line, n - __cfg_read_line, av, CFG_VV_FLAGS ) )
			continue;

		// return any useful data
		if( av->status == VV_LINE_ATTVAL )
			return 0;
	}

	return -1;
}



// step over one section
int config_ignore_section( FILE *fh )
{
	int rv;
	AVP a;

	while( ( rv = config_get_line( fh, &a ) ) == 0 )
		continue;

	return rv;
}



CCTXT *config_make_context( char *path, CCTXT *parent )
{
	CCTXT *ctx;

	ctx = (CCTXT *) allocz( sizeof( CCTXT ) );
	strncpy( ctx->file, path, 511 );

	if( parent )
	{
		ctx->parent = parent;

		if( parent->children )
			parent->children->next = ctx;
		parent->children = ctx;

		// inherit the section we were in
		memcpy( ctx->section, parent->section, 512 );
	}
	else
		strncpy( ctx->section, "main", 6 );

	// set the global on the first call
	if( !ctxt_top )
		ctxt_top = ctx;

	return ctx;
}



int config_file_dupe( CCTXT *c, char *path )
{
	CCTXT *ch;

	if( !c )
		return 0;

	if( !strcmp( path, c->file ) )
		return 1;

	for( ch = c->children; ch; ch = ch->next )
		if( config_file_dupe( ch, path ) )
			return 1;

	return 0;
}


int config_line( AVP *av )
{
	if( attIs( "tick_msec" ) )
		ctl->tick_usec = 1000 * atoi( av->val );
	else if( attIs( "daemon" ) )
	{
	  	if( valIs( "yes" ) || valIs( "y" ) || atoi( av->val ) )
			ctl->run_flags |= RUN_DAEMON;
		else
		  	ctl->run_flags &= ~RUN_DAEMON;
	}
	else if( attIs( "pidfile" ) )
	{
		free( ctl->pidfile );
		ctl->pidfile = config_relative_path( av->val );
	}
	else if( attIs( "basedir" ) )
	{
		free( ctl->basedir );
		ctl->basedir = str_copy( av->val, av->vlen );
	}
	return 0;
}


// always returns a new string
char *config_relative_path( char *inpath )
{
	char *ret;
	int len;

	if( *inpath != '~' )
		return strdup( inpath );

	if( !ctl->basedir )
		// just step over it
		return strdup( inpath + 1 );

	// add 1 for the /, remove 1 for the ~, add 1 for the \0
	len = strlen( ctl->basedir ) + strlen( inpath ) + 1;

	ret = (char *) malloc( len );
	snprintf( ret, len, "%s/%s", ctl->basedir, inpath + 1 );

	return ret;
}


#define	secIs( s )		!strcasecmp( context->section, s )

int config_read( char *inpath )
{
	int ret = 0, rv, lrv;
	FILE *fh = NULL;
	char *path;
	AVP av;

	// maybe add basedir
	path = config_relative_path( inpath );

	// check this isn't a duplicate
	if( config_file_dupe( ctxt_top, path ) )
	{
		warn( "Skipping duplicate config file '%s'.", path );
		free( path );
		return 0;
	}

	// set up our new context
  	context = config_make_context( path, context );

	debug( "Opening config file %s, section %s",
		path, context->section );

	// is this the first call
	if( !ctxt_top )
		ctxt_top = context;

	// die on not reading main config file, warn on others
	if( !( fh = fopen( path, "r" ) ) )
	{
		err("Could not open config file '%s' -- %s", path, Err );
	  	ret = -1;
		goto END_FILE;
	}

	while( ( rv = config_get_line( fh, &av ) ) != -1 )
	{
		// was it just a section change?
		if( rv > 0 )
			continue;

		// spot includes - they inherit context
		if( !strcasecmp( av.att, "include" ) )
		{
			if( config_read( av.val ) != 0 )
			{
				err( "Included config file '%s' invalid.", av.val );
				goto END_FILE;
			}
			continue;
		}

		// hand some sections off to different config fns
		if( secIs( "logging" ) )
			lrv = log_config_line( &av );
		else if( secIs( "network" ) )
		  	lrv = net_config_line( &av );
		else if( secIs( "memory" ) )
			lrv = mem_config_line( &av );
		else if( secIs( "stats" ) )
			lrv = stats_config_line( &av );
		else
			lrv = config_line( &av );

        ret += lrv;

		if( lrv )
		{
		  	err( "Bad config in file '%s', line %d",
				context->file, context->lineno );
			goto END_FILE;
		}
	}


END_FILE:
	if( fh )
		fclose( fh );

	debug( "Finished with config file '%s': %d", context->file, ret );

	// step our context back
	context = context->parent;

	free( path );
	return ret;
}

#undef secIs



MIN_CTL *config_create( void )
{
	MIN_CTL *c;

	c             = (MIN_CTL *) allocz( sizeof( MIN_CTL ) );
	c->log        = log_config_defaults( );
	c->locks      = lock_config_defaults( );
	c->mem        = mem_config_defaults( );
	c->net        = net_config_defaults( );
	c->stats      = stats_config_defaults( );

	c->cfg_file   = "conf/ministry.conf";
	c->pidfile    = strdup( DEFAULT_PID_FILE );

	c->tick_usec  = 1000 * DEFAULT_TICK_MSEC;

	gettimeofday( &(c->init_time), NULL );
	tvdupe( c->init_time, c->curr_time );

	return c;
}

