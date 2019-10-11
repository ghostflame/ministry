/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* http/init.c - functions to set up / stop httpd server                   *
*                                                                         *
* Updates:                                                                *
**************************************************************************/


#include "local.h"



int http_tls_load_file( TLS_FILE *sf, char *type )
{
	char desc[32];

	sf->type = str_dup( type, 0 );

	snprintf( desc, 32, "TLS %s file", sf->type );

	sf->len = MAX_TLS_FILE_SIZE;

	return read_file( sf->path, (char **) &(sf->content), &(sf->len), 1, desc );
}



int http_tls_setup( TLS_CONF *s )
{
	if( http_tls_load_file( &(s->cert), "cert" )
	 || http_tls_load_file( &(s->key),  "key"  ) )
		return -1;

	return 0;
}



// stomp on our password/length
void http_clean_password( HTTP_CTL *h )
{
	if( h->tls->password )
	{
		// scrub the data out of that field before freeing it
		// https://cwe.mitre.org/data/definitions/244.html
		explicit_bzero( (void *) h->tls->password, MAX_TLS_PASS_SIZE );
		free( (void *) h->tls->password );
	}

	h->tls->password = NULL;
	h->tls->passlen = 0;
}


int http_resolve_filters( void )
{
	HTTP_CTL *h = _http;

	if( h->web_iplist )
	{
		if( !( h->web_ips = iplist_find( h->web_iplist ) ) )
		{
			err( "Could not find requested HTTP IP filter list." );
			return -1;
		}
	}

	if( h->ctl_iplist )
	{
		if( !( h->ctl_ips = iplist_find( h->ctl_iplist ) ) )
		{
			err( "Could not find requested HTTP controls IP filter list." );
			return -1;
		}
	}

	return 0;
}




#define mop( _o )		MHD_OPTION_ ## _o

int http_start( void )
{
	HTTP_CTL *h = _http;
	uint16_t port;

	if( !h->enabled )
	{
		http_clean_password( h );
		return 0;
	}

	if( http_resolve_filters( ) != 0 )
	{
		http_clean_password( h );
		return -1;
	}

	// are we doing overall connect restriction?
	if( !h->web_ips )
		h->calls->policy = NULL;

	if( h->tls->enabled )
	{
		if( http_tls_setup( h->tls ) )
		{
			err( "Failed to load TLS certificates." );
			http_clean_password( h );
			return -1;
		}

		port = h->tls->port;
		h->proto = "https";
		flag_add( h->flags, MHD_USE_TLS );
	}
	else
	{
		port = h->port;
		h->proto = "http";
	}

	h->server_port = port;
	h->sin->sin_port = htons( port );

	MHD_set_panic_func( h->calls->panic, (void *) h );

	if( h->tls->enabled )
	{
		h->server = MHD_start_daemon( h->flags, 0,
			h->calls->policy,                (void *) h->web_ips,
			h->calls->handler,               (void *) h,
			mop( SOCK_ADDR ),                (struct sockaddr *) h->sin,
			mop( URI_LOG_CALLBACK ),         h->calls->reqlog, NULL,
			mop( EXTERNAL_LOGGER ),          h->calls->log, (void *) h,
			mop( NOTIFY_COMPLETED ),         h->calls->complete, NULL,
			mop( CONNECTION_LIMIT ),         h->conns_max,
			mop( PER_IP_CONNECTION_LIMIT ),  h->conns_max_ip,
			mop( CONNECTION_TIMEOUT ),       h->conns_tmout,
			mop( HTTPS_MEM_KEY ),            h->tls->key.content,
			mop( HTTPS_MEM_CERT ),           h->tls->cert.content,
#if MIN_MHD_PASS > 0
			mop( HTTPS_KEY_PASSWORD ),       (const char *) h->tls->password,
#endif
			mop( HTTPS_PRIORITIES ),         (const char *) h->tls->priorities,
			mop( END )
		);
	}
	else
	{
		h->server = MHD_start_daemon( h->flags, 0,
			h->calls->policy,                (void *) h->web_ips,
			h->calls->handler,               (void *) h,
			mop( SOCK_ADDR ),                (struct sockaddr *) h->sin,
			mop( URI_LOG_CALLBACK ),         h->calls->reqlog, NULL,
			mop( EXTERNAL_LOGGER ),          h->calls->log, (void *) h,
			mop( NOTIFY_COMPLETED ),         h->calls->complete, NULL,
			mop( CONNECTION_LIMIT ),         h->conns_max,
			mop( PER_IP_CONNECTION_LIMIT ),  h->conns_max_ip,
			mop( CONNECTION_TIMEOUT ),       h->conns_tmout,
			mop( END )
		);
	}

	// clean up the password variable
	http_clean_password( h );

	if( !h->server )
	{
		err( "Failed to start %s server on %hu: %s", h->proto, port, Err );
		return -1;
	}

	notice( "Started up %s server on port %hu.", h->proto, port );
	return 0;
}

#undef mop

void http_stop( void )
{
	HTTP_CTL *h = _http;

	if( h->server )
	{
		MHD_stop_daemon( h->server );
		pthread_mutex_destroy( &(h->hitlock) );
		notice( "Shut down %s server.", h->proto );
	}
}



int http_ask_password( void )
{
	static struct termios oldt, newt;
	HTTP_CTL *h = _http;
	volatile char *p;
	size_t bsz;
	int l;

	// can we even ask?
	if( !isatty( STDIN_FILENO ) )
	{
		err( "Cannot get a password - stdin is not a tty." );
		return 1;
	}

	// clear out any existing
	http_clean_password( h );
	h->tls->password = (volatile char *) allocz( MAX_TLS_PASS_SIZE );

	printf( "Enter TLS key password: " );
	fflush( stdout );

	// save terminal settings
	tcgetattr( STDIN_FILENO, &oldt );
	newt = oldt;

	// unset echoing
	newt.c_lflag &= ~(ECHO);

	// set terminal attributes
	tcsetattr( STDIN_FILENO, TCSANOW, &newt );

	// go get it - buffer is already zerod
	bsz = MAX_TLS_PASS_SIZE;
	l = (int) getline( (char **) &(h->tls->password), &bsz, stdin );

	// and fix the terminal
	tcsetattr( STDIN_FILENO, TCSANOW, &oldt );

	// and give us a fresh new line
	printf( "\n" );
	fflush( stdout );

	debug( "Password retrieved from command line." );

	// stomp on any newline and carriage return
	// and get the length
	p = h->tls->password + l - 1;
	while( l > 0 && ( *p == '\n' || *p == '\r' ) )
	{
		l--;
		*p-- = '\0';
	}
	h->tls->passlen = l;

	return 0;
}


