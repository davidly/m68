#include <errno.h>
#include <fcntl.h>

extern void *_mbot, *_mtop, *_mcur;

void *
brk(register void *ptr)
{
	ptr = (char *)(((long)ptr+3) & ~3);	/* set ptr on 32-bit boundary */
	if (ptr >= _mbot && ptr < _mtop){
		_mcur = ptr;
		return 0;
	} else {
		errno=ENOMEM;
		return (void *) -1;
	}
}

void *
sbrk(register size_t size)
{
	register void *ptr = _mcur;
	if (brk((char *)_mcur+size) != (void *)-1)
		return ptr;
	else 
		return (void *) -1;
}

