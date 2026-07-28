CC=cc
AS=as
CFLAGS=
AFLAGS=
O=oss

.c.$O:
	$(CC) -o $@ $(CFLAGS) $*.c
.a68.$O:
	$(AS) -o $@ $(AFLAGS) $*.a68
.asm.$O:
	$(AS) -o $@ $(AFLAGS) $*.asm

M8LIB=\
	floor.$O random.$O atan2.$O\
	fabs.$O frexp.$O ldexp.$O modf.$O pow.$O tran.$O fmod.$O \
	xflt68.$O

m8lib:	$(M8LIB) 

