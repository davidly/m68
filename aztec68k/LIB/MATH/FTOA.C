/* Copyright (C) 1984 by Manx Software Systems, Inc. */

static double round[] = { 10, 1,
	5e-1, 5e-2, 5e-3, 5e-4, 5e-5, 5e-6, 5e-7, 5e-8, 5e-9, 5e-10,
	5e-11, 5e-12, 5e-13, 5e-14, 5e-15, 5e-16 };

ftoa(number, buffer, maxwidth, flag)
double number; register char *buffer;
{
	register int i;
	int exp, digit, decpos, ndig;

	ndig = maxwidth+1;
	exp = 0;
	if (number < 0.0) {
		number = -number;
		*buffer++ = '-';
	}
	if (number > 0.0) {
		while (number < round[1]) {
			number *= round[0];
			--exp;
		}
		while (number >= round[0]) {
			number /= round[0];
			++exp;
		}
	}

	if (flag == 2) {		/* 'g' format */
		ndig = maxwidth;
		if (exp < -4 || exp > maxwidth)
			flag = 0;		/* switch to 'e' format */
	} else if (flag == 1)	/* 'f' format */
		ndig += exp;

	if (ndig >= 0) {
		if ((number += round[(ndig>16?16:ndig)+1]) >= round[0]) {
			number = round[1];
			++exp;
			if (flag)
				++ndig;
		}
	}

	if (flag) {
		if (exp < 0) {
			*buffer++ = '0';
			*buffer++ = '.';
			i = -exp - 1;
			if (ndig <= 0)
				i = maxwidth;
			while (i--)
				*buffer++ = '0';
			decpos = 0;
		} else {
			decpos = exp+1;
		}
	} else {
		decpos = 1;
	}

	if (ndig > 0) {
		for (i = 0 ; ; ++i) {
			if (i < 16) {
				digit = (int)number;
				*buffer++ = digit+'0';
				number = (number - digit) * round[0];
			} else
				*buffer++ = '0';
			if (--ndig == 0)
				break;
			if (decpos && --decpos == 0)
				*buffer++ = '.';
		}
	}

	if (!flag) {
		*buffer++ = 'e';
		if (exp < 0) {
			exp = -exp;
			*buffer++ = '-';
		} else
			*buffer++ = '+';
		if (exp >= 100) {
			*buffer++ = exp/100 + '0';
			exp %= 100;
		}
		*buffer++ = exp/10 + '0';
		*buffer++ = exp%10 + '0';
	}
	*buffer = 0;
}

