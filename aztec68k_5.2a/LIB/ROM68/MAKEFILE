CC=c68
AS=as68
CFLAGS=
AFLAGS=
O=oss

.c.$O:
	$(CC) -o $@ $(CFLAGS) $*.c
.a68.$O:
	$(AS) -o $@ $(AFLAGS) $*.a68

CLIB=\
	rom68.$O sbrk.$O stkchk.$O _main.$O _exit.$O \
	open.$O _open.$O close.$O _close.$O read.$O _read.$O write.$O _write.$O \
	lseek.$O _lseek.$O ioctl.$O _ioctl.$O isatty.$O _isatty.$O \
	_rename.$O unlink.$O _unlink.$O 

clib:	$(CLIB)
