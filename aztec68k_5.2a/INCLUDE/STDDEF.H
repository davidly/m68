/* Copyright 1987 Manx Software Systems, Inc */

#ifndef __STDDEF_H 
#define __STDDEF_H

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned long size_t;
#endif

#ifndef _PTRDIFF_T
#define _PTRDIFF_T
typedef long ptrdiff_t;
#endif

#ifndef _WCHAR_T
#define _WCHAR_T
typedef char wchar_t;
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

extern int errno;

#define offsetof(type,memb) ((size_t)((unsigned long)&((type *)0)->memb))

#endif

