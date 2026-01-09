#include "errno.h"

#define MAXEXP	1024
#define MINEXP	-1023

#define PI		3.14159265358979323846
#define PIov2	1.57079632679489661923

double atan2(v,u)
double u,v;
{
	double f, frexp(), fabs(), atan();
	int vexp, uexp;
	extern int flterr;
	extern int errno;

	if (u == 0.0) {
		if (v == 0.0) {
			errno = EDOM;
			return 0.0;
		} else if (v > 0.0 )
			return PIov2;
		return -PIov2;
	}

	frexp(v, &vexp);
	frexp(u, &uexp);
	if (vexp-uexp > MAXEXP-3)	/* overflow */
		f = PIov2;
	else {
		if (vexp-uexp < MINEXP+3)	/* underflow */
			f = 0.0;
		else
			f = atan(fabs(v/u));
		if (u < 0.0)
			f = PI - f;
	}
	if (v < 0.0)
		f = -f;
	return f;
}

