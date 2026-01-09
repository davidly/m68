/* Copyright (C) 1983 by Manx Software Systems */
#include	<ctype.h>

double
atof(cp)
register char *cp;
{
	double acc, zero = 0.0, ten = 10.0;
	int msign, esign, dpflg;
	int i, dexp;

	while (*cp == ' ' || *cp == '\t')
		++cp;
	if (*cp == '-') {
		++cp;
		msign = 1;
	} else {
		msign = 0;
		if (*cp == '+')
			++cp;
	}
	dpflg = dexp = 0;
	for (acc = zero ; ; ++cp) {
		if (isdigit(*cp)) {
			acc *= ten;
			acc += *cp - '0';
			if (dpflg)
				--dexp;
		} else if (*cp == '.') {
			if (dpflg)
				break;
			dpflg = 1;
		} else
			break;
	}
	if (*cp == 'e' || *cp == 'E') {
		++cp;
		if (*cp == '-') {
			++cp;
			esign = 1;
		} else {
			esign = 0;
			if (*cp == '+')
				++cp;
		}
		for ( i = 0 ; isdigit(*cp) ; i = i*10 + *cp++ - '0' )
			;
		if (esign)
			i = -i;
		dexp += i;
	}
	if (dexp < 0) {
		while (dexp++)
			acc /= ten;
	} else if (dexp > 0) {
		while (dexp--)
			acc *= ten;
	}
	if (msign)
		acc = -acc;
	return acc;
}

