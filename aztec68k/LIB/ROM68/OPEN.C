#include	<fcntl.h>

creat(name, mode)
char *name;
{
	return open(name, O_WRONLY|O_TRUNC|O_CREAT, mode);
}

open(uname, flags, mode)
char *uname;
{
}

