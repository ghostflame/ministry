/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* io/tls.c - handles tls setup and teardown                               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "local.h"


int io_tls_write_data( SOCK *s, int off )
{
	int rv, len, sent;
	char *ptr;

	sent = 0;
	ptr  = s->out->bf->buf + off;
	len  = s->out->bf->len - off;

	while( len )
	{
		rv = gnutls_record_send( s->tls->sess, ptr, len );

		if( rv < 0 )
		{
			// on these errors, just try again with the same values
			if( rv == GNUTLS_E_INTERRUPTED || rv == GNUTLS_E_AGAIN )
				continue;

			warn( "Could not send record on TLS channel -- %s", gnutls_strerror( rv ) );
			flagf_add( s, IO_CLOSE );
			break;
		}

		len  -= rv;
		ptr  += rv;
		sent += rv;
	}

	return sent;
}


int io_tls_connect( SOCK *s )
{
	const char *ep = NULL;
	int rv, et, st;
	IOTLS *t;

	t = s->tls;

	if( t->conn )
		io_disconnect( s, 1 );

	if( io_connect( s ) < 0 )
		return -1;

	//debug( "Socket fd: %d", s->fd );

	// looks like we need to do all this again on reconnect
	if( ( rv = gnutls_init( &(t->sess), GNUTLS_CLIENT ) ) < 0 )
	{
		err( "Failed to init TLS client session -- %s", gnutls_strerror( rv ) );
		return -1;
	}

	// validates the certificate against this name
	// TODO permit targets to set a peer name that overrides their hostname
	if( t->peer )
	{
		gnutls_server_name_set( t->sess, GNUTLS_NAME_DNS, t->peer, t->plen );
		debug( "Set TLS peer to %s", t->peer );
	}

	// turn off stuff we don't like
	// TODO embrace 1.3 at some point
	rv = gnutls_priority_set_direct( t->sess, "SECURE256:!VERS-TLS1.1:!VERS-TLS1.0:!VERS-SSL3.0:%SAFE_RENEGOTIATION", &ep );
	if( rv != GNUTLS_E_SUCCESS )
	{
		warn( "Failed to set gnutls priorities - error at: %s -- %s", ep, gnutls_strerror( rv ) );
		gnutls_set_default_priority( t->sess );
	}

	// we may set more credentials in later code - client certs
	gnutls_credentials_set( t->sess, GNUTLS_CRD_CERTIFICATE, t->cred );

	if( flagf_has( t, IO_TLS_VERIFY ) )
	{
		debug( "Setting verification on remote target endpoint." );
		gnutls_session_set_verify_cert( t->sess, t->peer, 0 );
	}

	// give the session the connected socket
	gnutls_transport_set_int( t->sess, s->fd );
	gnutls_handshake_set_timeout( t->sess, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT );

	// loop around the handshake
	do
	{
		rv = gnutls_handshake( t->sess );
	}
	while( rv < 0 && gnutls_error_is_fatal( rv ) == 0 );

	//debug( "Session handshake finished -- %d", rv );

	if( rv < 0 )
	{
		// this should only happen if we ask for verification
		if( rv == GNUTLS_E_CERTIFICATE_VERIFICATION_ERROR )
		{
			et = gnutls_certificate_type_get( t->sess );
			st = gnutls_session_get_verify_cert_status( t->sess );
			gnutls_certificate_verification_status_print( st, et,
				&(t->dt), 0 );
			err( "Error with certificate during handshake -- %s", t->dt.data );
			gnutls_free( t->dt.data );
		}
		else
			err( "TLS handshake failed -- %s", gnutls_strerror( rv ) );

		io_disconnect( s, 1 );
		return -1;
	}
	else
		t->conn = 1;

	return 0;
}

int io_tls_disconnect( SOCK *s )
{
	if( s->tls->conn )
	{
		gnutls_bye( s->tls->sess, GNUTLS_SHUT_RDWR );
		gnutls_deinit( s->tls->sess );
		s->tls->conn = 0;
	}

	return 0;
}


IOTLS *io_tls_make_session( uint32_t flags, char *peername )
{
	IOTLS *t;

	t = (IOTLS *) allocz( sizeof( IOTLS ) );
	gnutls_certificate_allocate_credentials( &(t->cred) );
	if( gnutls_certificate_set_x509_system_trust( t->cred ) < 0 )
		warn( "Failed to import system CA set." );

	t->flags = flags;

	if( peername )
	{
		t->plen = strlen( peername );
		t->peer = str_dup( peername, t->plen );
	}

	t->init = 1;
	t->conn = 0;

	return t;
}


void io_tls_end_session( SOCK *s )
{
	IOTLS *t = s->tls;

	if( t->init )
	{
		if( t->conn )
			io_tls_disconnect( s );

		gnutls_certificate_free_credentials( t->cred );
		t->init = 0;
	}
}
