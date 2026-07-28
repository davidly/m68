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
	abs.$O atexit.$O atoi.$O atol.$O \
	bsearch.$O calloc.$O div.$O exit.$O free.$O \
	getenv.$O labs.$O malloc.$O mblen.$O mbtowc.$O \
	qsort.$O rand.$O realloc.$O strtol.$O \
	strtoul.$O system.$O wctomb.$O ldiv.$O \
	abort.$O wcstombs.$O mbstowcs.$O

MLIB=\
	atof.$O ftoa.$O strtod.$O strtold.$O 

clib:	$(CLIB)

mlib:	$(MLIB)

mslib:	$(MLIB)

m8lib:	$(MLIB)

malib:  $(MLIB)

mflib:  $(MLIB)

malloc.$O:	rommallo.c amimallo.c macmallo.c

free.$O:	romfree.c amifree.c macfree.c

