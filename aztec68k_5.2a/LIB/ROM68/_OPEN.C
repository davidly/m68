#include	<fcntl.h>

int
_creat(const char *name, int mode)
{
	return _open(name, O_WRONLY|O_TRUNC|O_CREAT, mode);
}

int
_open(register const char *name, int flag, ...)
{
	return -1;
}
