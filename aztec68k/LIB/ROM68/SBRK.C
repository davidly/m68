#include <errno.h>

extern char *_mbot, *_mtop, *_mcur;

char *
brk(ptr)
register char *ptr;
{
	ptr = (char *)(((long)ptr+3) & ~3);	/* set ptr on 32-bit boundary */
	if (ptr >= _mbot && ptr < _mtop){
		_mcur = ptr;
		return 0;
	} else {
		errno=ENOMEM;
		return (char *) -1;
	}
}

char *
sbrk(size)
register unsigned size;
{
	register char *ptr = _mcur;
	if (brk(_mcur+size) != (char *)-1)
		return ptr;
	else 
		return (char *) -1;
}

