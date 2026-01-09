/* Copyright (C) 1984 by Manx Software Systems, Inc. */

int (*cls_)() = 0;

exit(code)
{
	register int fd;

	if (cls_)
		(*cls_)();
	for (fd = 0 ; fd < 10 ; fd++)
		close(fd);
	_exit(code);
}

