/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

#ifndef __STDLIB_H
#define __STDLIB_H

# ifndef _SIZE_T
# define _SIZE_T
  typedef unsigned long size_t;
# endif

#ifndef _WCHAR_T
#define _WCHAR_T
typedef char wchar_t;
#endif

# ifndef ERANGE
#define ERANGE 13
# endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#define EXIT_FAILURE (-1)
#define EXIT_SUCCESS 0

#ifndef HUGE_VAL
#ifdef _FLT_FFP
#define HUGE_VAL	9.22337177E+17
#else
#define HUGE_VAL	1.797693134862316E+308
#endif
#endif

#define RAND_MAX 32767

#define	MB_CUR_MAX	1
#ifndef MB_LEN_MAX
#define	MB_LEN_MAX	1
#endif

typedef struct {
	int quot;
	int rem;
} div_t;						/* quotient and remainder for div() */

typedef struct {
	long quot;
	long rem;
} ldiv_t;						/* quotient and remainder for ldiv() */

double atof(const char *_nptr);
int atoi(const char *_nptr);
long int atol(const char *_nptr);
double strtod(const char *_nptr, char **_endptr);
long int strtol(const char *_nptr, char **_endptr, int _base);
unsigned long int strtoul(const char *_nptr, char **_endptr, int _base);

int rand(void);
void srand(unsigned int _seed);

void *calloc(size_t _nmemb, size_t _size);
void free(void *_ptr);
void *malloc(size_t _size);
void *realloc(void *_ptr, size_t _size);

void abort(void);
int atexit(void (*_func)(void));
void exit(int _status);
char *getenv(const char *_name);
int system(const char *_string);

void *bsearch(const void *_key, const void *_base, size_t _nmemb, size_t _size,
								int (*_compar)(const void *, const void *));
void qsort(void *_base, size_t _nmemb, size_t _size,
								int (*_compar)(const void *, const void *));

int abs(int _j);
div_t div(int _numer, int _denom);
long int labs(long int _j);
ldiv_t ldiv(long int _numer, long int _denom);

int mblen(const char *_s, size_t _n);
int mbtowc(wchar_t *_pwc, const char *_s, size_t _n);
int wctomb(char *_s, wchar_t _wchar);
size_t mbstowcs(wchar_t *_pwcs, const char *_s, size_t _n);
size_t wcstombs(char *_s, const wchar_t *_pwcs, size_t _n);

#if !__STDC__
/* non ANSI C functions */
void ftoa(double _val, char *_buf, int, int);
long double strtold(const char *_nptr, char **_endptr);
#endif	/* !__STDC__ */

#endif	/* _STDLIB_H */
