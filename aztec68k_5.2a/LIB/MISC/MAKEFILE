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

CLIB=\
	assert.$O ctype.$O isalnum.$O isalpha.$O isascii.$O \
	iscntrl.$O isdigit.$O isgraph.$O islower.$O isprint.$O \
	ispunct.$O isspace.$O isupper.$O isxdigit.$O setlocal.$O \
	localeco.$O signal.$O ldiv.$O lmul.$O peek.$O \
	setjmp.$O toupper.$O tolower.$O

clib:	$(CLIB)

signal.$O:	romsig.c amisig.c macsig.c
