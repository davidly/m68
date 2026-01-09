/* Copyright (c) 1986 by Manx Software Systems Inc. */

long
time(tloc)
long *tloc;
{
	long x;

	GetDateTime(&x);

	if (tloc)
		*tloc = x;

	return x;
}
