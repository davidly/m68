/* Copyright Manx Software Systems, Inc. 1988.  All rights reserved */

#ifndef __MATH_H
#define __MATH_H

double frexp(double, int *);
double ldexp(double, int);
double modf(double, double *);
double cotan(double);
double log10(double);
double log(double);
double pow(double, double);
double sqrt(double);
double fabs(double);
double fmod(double, double);
double sin(double);
double cos(double);
double tan(double);
double exp(double);
double floor(double);
double ceil(double);
double sinh(double);
double cosh(double);
double tanh(double);
double asin(double);
double acos(double);
double atan(double);
double atan2(double, double);

#ifdef	HUGE_VAL
#undef	HUGE_VAL
#endif

#ifdef _FLT_FFP	/* Motorola FFP format */
#define HUGE_VAL	9.22337177E+17
#else			/* IEEE format */
#define HUGE_VAL	1.797693134862316E+308
#endif

#if !__STDC__

#ifdef _FLT_FFP		/* Motorola FFP format */
#define HUGE		HUGE_VAL
#define LOGHUGE		(709.778)
#define TINY_VAL	(5.42101070E-20)
#define TINY		TINY_VAL
#define LOGTINY		(-708.396)
#else				/* IEEE format */
#define HUGE		HUGE_VAL
#define LOGHUGE		(709.778)
#define TINY_VAL	(2.2e-308)
#define TINY		TINY_VAL
#define LOGTINY		(-708.396)
#endif

void sran(long seed);
double ran(void);
double randl(double x);
void ftoa(double number, char *buffer, int maxwidth, int mode);
#define power(x,y) pow(x,y)
#endif	/* !__STDC__ */

#endif
