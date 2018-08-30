/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* curlw.h - defines curl wrapper functions                                *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef SHARED_CURLW_H
#define SHARED_CURLW_H


#define CErr		curl_easy_strerror( cc )

// curl to file, deleting it before writing if required

// TODO - change the options to flags?
int curlw_to_file( char *url, int secure, int ssl, int validate, FILE *fh )
{
	


}



#endif

