/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2015 John Denholm                                             *
*                                                                         *
* tree.h - main data tree structure definitions                           *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef ARCHIVIST_TREE_H
#define ARCHIVIST_TREE_H


struct tree_leaf
{
	char			*	filepath;	// ?
	int					fd;
};



struct tree_element
{
	TEL				*	next;
	TEL				*	child;
	TEL				*	parent;

	char			*	name;
	char			*	path;
	LEAF			*	leaf;

	uint16_t			len;
	uint16_t			plen;
};


struct tree_control
{
	TEL				*	root;
	SSTR			*	thash;		// lookup table

};

TREE_CTL *tree_config_defaults( void );
conf_line_fn tree_config_line;


#endif
