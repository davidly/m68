/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

/*
 *	Synopsis
 *
 *	double atof(const char *nptr);
 *
 *
 *	Description
 *
 *		The atof function converts the initial portion of the string pointed to
 *	by nptr to double representation. Except for the behavior on error, it is
 *	equivalent to 
 *
 *		strtod(nptr, (char **)NULL);
 *
 *
 *	Returns
 *
 *		The atof funciton returns the converted value.
 */

#include <stdlib.h>

double
atof(register const char *cp)
{
	return(strtod(cp, (char **)0));
}

