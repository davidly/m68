/* Copyright Manx Software Systems, Inc. 1988.  All rights reserved */

#ifndef __STDARG_H
#define __STDARG_H

typedef char *va_list;

/*	changed because
	#define va_start(ap, parmN) ((ap) = (char *)(&parmN + 1)) 
	doesn't work with parmN of type char
*/
	
#define va_start(ap, parmN) \
	((ap) = (char *)((unsigned long)(&parmN + 1)+1 &~1L))
#define va_arg(ap, type) ((ap) += sizeof(type), ((type *)(ap))[-1])
#define va_end(ap)

#endif
