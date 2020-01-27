/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* tree.c - functions for manipulating the metric tree                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "archivist.h"



TREE_CTL *tree_config_defaults( void )
{
	TREE_CTL *t = (TREE_CTL *) allocz( sizeof( TREE_CTL ) );



	return t;
}

int tree_config_line( AVP *av )
{
	return -1;
}

