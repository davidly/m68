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
	clearerr.$O fclose.$O fdopen.$O feof.$O ferror.$O \
	fgetc.$O fgetpos.$O fgets.$O filbuf.$O \
	fileopen.$O fopen.$O format.$O fprintf.$O \
	fputc.$O fputs.$O fread.$O freopen.$O fscanf.$O \
	fseek.$O fsetpos.$O ftell.$O fwrite.$O getbuf.$O \
	getchar.$O gets.$O getw.$O perror.$O printf.$O putchar.$O \
	puts.$O putw.$O remove.$O rename.$O rewind.$O scan.$O \
	scanf.$O setbuf.$O setvbuf.$O sprintf.$O sscanf.$O \
	tmpfile.$O tmpnam.$O ungetc.$O vfprintf.$O vprintf.$O \
	vsprintf.$O _format.$O

MLIB=\
	format.$O vsprintf.$O vprintf.$O vfprintf.$O sprintf.$O \
	printf.$O fprintf.$O scan.$O sscanf.$O scanf.$O \
	fscanf.$O _format.$O

clib:	$(CLIB)

mlib:	$(MLIB)

mslib:	$(MLIB)

m8lib:	$(MLIB)

malib:  $(MLIB)

mflib:  $(MLIB)


