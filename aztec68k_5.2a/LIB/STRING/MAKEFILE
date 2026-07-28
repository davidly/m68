CC=cc
AS=as
CFLAGS=
AFLAGS=
O=oss

.c.$O:
	$(CC) -o $@ $(CFLAGS) $*.c
.a68.$O:
	$(AS) -o $@ $(AFLAGS) $*.a68

CLIB=\
	index.$O  memchr.$O memcmp.$O memcpy.$O memccpy.$O memmove.$O \
	memset.$O movmem.$O rindex.$O strcat.$O strchr.$O \
	strcmp.$O strcpy.$O strcspn.$O strerror.$O strlen.$O \
	strncat.$O strncmp.$O strncpy.$O strpbrk.$O strrchr.$O \
	strspn.$O strstr.$O strtok.$O strxfrm.$O swapmem.$O \
	setmem.$O strdup.$O strcoll.$O stricmp.$O strnicmp.$O \
	strlwr.$O strupr.$O

clib:	$(CLIB)

