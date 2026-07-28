/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

/*
 *	Synopsis
 *
 *	int system(const char *string);
 *
 *
 *	Description
 *
 *		The system function passes the string pointed to by string to the host
 *	environment to be executed by a "command processor" in an implementation-
 *	defined manner. A null pointer may be used for string to inquire whether a
 *	command processor exists.
 *
 *
 *	Returns
 *
 *		If the argument is a null pointer, the system function returns nonzero
 *	only if a command processor is available. If the argument is not a null
 *	pointer, the system function returns an implementation-defined value.
 */

#include <stdlib.h>

int system(const char *string)
{
	return(0);
}

