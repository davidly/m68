/* Copyright (C) 1981,1982,1983 by Manx Software Systems */
#include <ctype.h>

#if MPU8080 || MPUZ80
char *_fmtcvt();
#else

static char *
_fmtcvt(ap, base, cp, len)
int *ap; register char *cp;
{
	register unsigned long val;
	static char digits[]="0123456789abcdef";

	if (len == sizeof(long))
		val = *(long *)ap;
	else if (base > 0)
		val = *(unsigned *)ap;
	else
		val = *ap;

	len = 0;
	if (base < 0) {
		base = -base;
		if ((long)val < 0) {
			val = -val;
			len = 1;
		}
	}

	do {
		*--cp = digits[(int)(val%base)];
	} while ((val /= base) != 0);
	if (len)
		*--cp = '-';
	return cp;
}
#endif

format(putsub, fmt, argp)
register int (*putsub)(); register char *fmt; char *argp;
{
	register int c;
	union {
		int *ip;
		char *cp;
		char **cpp;
#ifdef FLOAT
		double *dp;
#endif
	} args; 
	int charcount;
	int rj, fillc;
	int maxwidth, width;
	int i, k;
	char *cp;
	auto char s[200];

	charcount = 0;
	args.cp = argp;
	while ( c = *fmt++ ) {
		if ( c == '%' ) {
			s[14] = 0;
			rj = 1;
			fillc = ' ';
			maxwidth = 10000;
			if ((c = *fmt++) == '-') {
				rj = 0;
				c = *fmt++;
			}
			if (c == '0') {
				fillc = '0';
				c = *fmt++;
			}
			if (c == '*') {
				width = *args.ip++;
				c = *fmt++;
			} else {
				for (width = 0 ; isdigit(c) ; c = *fmt++)
					width = width*10 + c - '0';
			}
			if ( c == '.' ) {
				if ((c = *fmt++) == '*') {
					maxwidth = *args.ip++;
					c = *fmt++;
				} else {
					for (maxwidth = 0 ; isdigit(c) ; c = *fmt++)
						maxwidth = maxwidth*10 + c - '0';
				}
			}
			i = sizeof(int);
			if (c == 'l') {
				c = *fmt++;
				i = sizeof(long);
			} else if (c == 'h')
				c = *fmt++;

			switch ( c ) {
			case 'o':
				k = 8;
				goto do_conversion;
			case 'u':
				k = 10;
				goto do_conversion;
			case 'x':
				k = 16;
				goto do_conversion;

			case 'd':
				k = -10;
	do_conversion:
				cp = _fmtcvt(args.cp, k, s+14, i);
				args.cp += i;
				break;

			case 's':
				i = strlen(cp = *args.cpp++);
				goto havelen;
#ifdef FLOAT
			case 'e':
			case 'f':
			case 'g':
				ftoa(*args.dp++, s, maxwidth==10000?6:maxwidth, c-'e');
				i = strlen(cp = s);
				maxwidth = 200;
				goto havelen;
#endif

			case 'c':
				c = *args.ip++;
			default:
				*(cp = s+13) = c;
				break;
			}

			i = (s+14) - cp;
		havelen:
			if ( i > maxwidth )
				i = maxwidth;
			
			if ( rj ) {
				if ((*cp == '-' || *cp == '+') && fillc == '0') {
					--width;
					if ((*putsub)(*cp++) == -1)
						return -1;
				}
				for (; width-- > i ; ++charcount)
					if ((*putsub)(fillc) == -1)
						return -1;
			}
			for ( k = 0 ; *cp && k < maxwidth ; ++k )
				if ((*putsub)(*cp++) == -1)
					return -1;
			charcount += k;
			
			if ( !rj ) {
				for (; width-- > i ; ++charcount)
					if ((*putsub)(' ') == -1)
						return -1;
			}
		} else {
			if ((*putsub)(c) == -1)
				return -1;
			++charcount;
		}
	}
	return charcount;
}

