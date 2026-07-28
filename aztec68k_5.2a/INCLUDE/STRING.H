/* Copyright 1989 Manx Software Systems, Inc. All rights reserved */

#ifndef __STRING_H
#define __STRING_H

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned long size_t;
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

void *memcpy(void *_dst, const void *_src, size_t _n);
void *memmove(void *_dst,const void *_src, size_t _n);
char *strcpy(char *_dst, const char *_src);
char *strncpy(char *_dst, const char *_src, size_t _n);
char *strcat(char *_dst, const char *_src);
char *strncat(char *_dst, const char *_src, size_t _n);

void *memchr(const void *_s, int _c, size_t _n);
void *memset(void *_s, int _c, size_t _n);
int memcmp(const void *_s1, const void *_s2, size_t _n);
int strcmp(const char *_s1, const char *_s2);
int strncmp(const char *_s1, const char *_s2, size_t _n);
char *strpbrk(const char *_s1, const char *_s2);
char *strchr(const char *_s, int _c);
char *strrchr(const char *_s, int _c);
char *strstr(const char *_s1, const char *_s2);
char *strtok(char *_s1, const char *_s2);
char *strerror(int _errnum);
size_t strcspn(const char *_s1, const char *_s2);
size_t strlen(const char *_s);
size_t strspn(const char *_s1, const char *_s2);
int strcoll(const char *_s1, const char *_s2);
size_t strxfrm(char *_s1, const char *_s2, size_t _n);

#if !__STDC__ /* non ANSI C functions */

char *index(char *_s, int _c);
void *memccpy(void *_dst, const void *_src, int _c, size_t _n);
void movmem(const void *src, void *dst, size_t n);
char *rindex(char *_s, int _c);
void swapmem(void *_s1, void *_s2, size_t _n);
void setmem(void *s, size_t n, int c);
char *strdup(char *_s);
int stricmp(const char *_s1, const char *_s2);
int strnicmp(const char *_s1, const char *_s2, size_t _n);
char *strlwr (char *_s1);
char *strupr (char *_s2);

#endif /* !__STDC__ */

#endif /* _STRING_H */

