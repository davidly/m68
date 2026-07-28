/* Copyright (C) 1984 by Manx Software Systems, Inc. */

#include <math.h>

#ifdef _FLT_FFP
static long lround[] = {
	0xa0000044L, 0x80000041L, 0xa0000043L,
	0x80000040L, 0xcccccd3cL, 0xa3d70a39L, 0x83126e36L,
	0xd1b71632L, 0xa7c5ab2fL, 0x8637bc2cL, 0xd6bf9328L,
	0xabcc7625L, 0x89705e22L, 0xdbe6fd1eL, 0xafebfe1bL,
	0x8cbccb18L, 0xe12e1214L, 0xb424db11L, 0x901d7c0eL,
};
#else
static double round[] = { 10, 1, 5,
	5e-1, 5e-2, 5e-3, 5e-4, 5e-5, 5e-6, 5e-7, 5e-8, 5e-9, 5e-10,
	5e-11, 5e-12, 5e-13, 5e-14, 5e-15, 5e-16 };
#endif

void
ftoa(double number, register char *buffer, int maxwidth, int mode)
{
	register int i;
	register int exp, digit, decpos, ndig;
	register int flag;
#ifdef _FLT_FFP
	register float *round = (float *)lround;
#endif

#ifndef _FLT_FFP
	if ((*(short *)&number&0x7ff0) == 0x7ff0) {
		exp = (*(short *)&number&0x8000) ? '-' : '+';
		while (maxwidth--)
			*buffer++ = exp;
		*buffer = 0;
		return;
	}
#endif

	/*
	 *	Convert the number to a number between 1 and 10 or 0. Calculate the
	 *	exponent at the same time.
	 */

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


	/*
	 *	The number of digits to display is equal to the number of digits
	 *	following the decimal point (maxwidth) + digits to the left (exp) +
	 *	a guaranteed digit since the number 10 > x >= 1. For E format, the
	 *	exp digits are not counted, while G format works differently.
	 */

	flag = mode & 3;
	if (flag == 2) {						/* 'g' format */
		if (maxwidth == 0)
			maxwidth = 1;
		if (exp < -4 || exp >= maxwidth)
			flag = -1;						/* switch to 'e' format */
		ndig = maxwidth;
	} else if (flag == 1)					/* 'f' format */
		ndig = maxwidth + exp + 1;
	else									/* 'e' format */
		ndig = maxwidth + 1;

	/*
	 *	Round the number up using 5e-x where x is one more than the
	 *	precision of the number.
	 */

	if (ndig >= 0) {
		if ((number += round[(ndig>16?16:ndig)+2]) >= round[0]) {
			number = round[1];
			++exp;
			if (flag > 0)
				++ndig;
		}
	}

	if (flag > 0) {
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

	if (decpos && (mode&0x20))
		*buffer++ = '.';
	if (flag <= 0) {
		*buffer++ = (mode&0x10)?'E':'e';
		if (exp < 0) {
			exp = -exp;
			*buffer++ = '-';
		} else
			*buffer++ = '+';
		*buffer++ = exp/100 + '0';
		exp %= 100;
		*buffer++ = exp/10 + '0';
		*buffer++ = exp%10 + '0';
	}
	if (decpos == 0 && (flag == 2 || flag == -1) && (mode&0x20) == 0) {
		while (*--buffer == '0')
			;
		if (*buffer != '.')
			++buffer;
	}
	*buffer = 0;
}

