/* Copyright Manx Software Systems, Inc. 1988.  All rights reserved */

#ifndef __STAT_H 
#define __STAT_H 

struct stat {
	char st_attr;
	long st_mtime;
	long st_size;
#ifdef MCH_MACINTOSH
	long st_rsize;
#endif
};

/* settings of the st_attr field */
#ifdef MCH_MACINTOSH
#define ST_RDONLY	0x01	/* file is locked or read only */
#define	ST_ORSRC	0x04	/* resource fork is open */
#define	ST_ODATA	0x08	/* data fork is open */
#define ST_DIRECT	0x10	/* file is a sub-directory */
#define	ST_OFILE	0x80	/* file is in use or open */
#else
#define	ST_DELETE	0x01
#define	ST_EXECUTE	0x02
#define	ST_WRITE	0x04
#define	ST_READ		0x08
#endif

/* st_mtime contains system time, which is...
 * ... on Macintosh, seconds since January 1, 1904
 * ... on Amiga, seconds since Jan 1, 1978
 * TIME_MCH2AZTEC can be added to st_mtime to convert system time
 * to time_t format
 */

#ifndef TIME_MCH2AZTEC
#if MCH_MACINTOSH
#define TIME_MCH2AZTEC (-2082844800L)
#elif MCH_AMIGA
#define TIME_MCH2AZTEC (2*(4*365L+1)*(60*60*24L))	/* 2 googles in 8 years */
#else
#define TIME_MCH2AZTEC 0
#endif
#endif

int stat(char *name, struct stat *buf);

#if !__STDC__ /* non ANSI C user-visible stuff */

int _access(char *name, int mode);

#endif /* !__STDC__ */

#endif

