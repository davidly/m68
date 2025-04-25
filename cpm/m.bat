@echo off

del %1.68k 2>nul
..\m68 cp68.68k %1.c %1.i
..\m68 c068.68k %1.i %1.1 %1.2 %1.3 -f
del %1.i
..\m68 c168.68k %1.1 %1.2 %1.s
del %1.1
del %1.2

..\m68 as68.68k -l -u %1.s
del %1.s

..\m68 lo68.68k -r -u_nofloat -o %1.68k s.o %1.o clib
del %1.o


