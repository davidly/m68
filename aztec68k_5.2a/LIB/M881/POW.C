#include "math.h"
#include "errno.h"

double pow(double a, double b)
{
	double answer, tmp;
	extern int errno;
	char sign;
	
	if (a == 0) {
		if (b <= 0)
domain:		errno = EDOM;
		return 0.0;
	}
	if (b == 0)
		return 1.0;		/* anything raised to 0 is 1 */
	sign = 0;
	if (modf(b,&answer) == 0) {
		if (a < 0)
			sign = 1, a = -a;
		if (answer < 0)
			answer = -answer;
		modf(answer/2, &tmp);
		if (tmp*2 == answer)
			sign = 0;		/* number is even so sign is positive */

	} else if (a < 0)
		goto domain;

	answer = exp(log(a)*b);
	return sign ? -answer : answer;
}

