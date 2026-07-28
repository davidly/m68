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

MLIB=\
	asin.$O atan.$O exp.$O floor.$O fmod.$O \
	log.$O pow.$O random.$O sin.$O sinh.$O \
	sqrt.$O tan.$O tanh.$O\
	dtof.$O dtox.$O fabs.$O floats.$O flt68.$O \
	frexp.$O ftod.$O ftox.$O ldexp.$O modf.$O \
	regs.$O x80to96.$O x96to80.$O xflt68.$O xtod.$O \
	xtof.$O
	
MSLIB=\
	x80to96.$O x96to80.$O floats.$O

mlib:	$(MLIB)

mslib:	$(MSLIB)
