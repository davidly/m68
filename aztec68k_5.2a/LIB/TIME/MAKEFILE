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
	asctime.$O clock.$O ctime.$O gmtime.$O \
	localtim.$O mktime.$O strftime.$O time.$O

MLIB=\
	difftime.$O

clib:	$(CLIB)

mlib:	$(MLIB)

mslib:	$(MLIB)

m8lib:	$(MLIB)

malib: $(MLIB)

mflib: $(MLIB)

