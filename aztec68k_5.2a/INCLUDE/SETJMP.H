/* Copyright Manx Software Systems, Inc. 1988.  All rights reserved */

#ifndef __SETJMP_H
#define __SETJMP_H

#ifdef  MPU68000
#define __JBUFSIZE  (15*sizeof(char *))
#else
#define __JBUFSIZE	(6*sizeof(char *))
#endif

typedef char jmp_buf[__JBUFSIZE];

int setjmp(jmp_buf _env);
void longjmp(jmp_buf _env, int _val);

#endif

