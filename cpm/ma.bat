@echo off

del %1.68K 2>nul
..\m68 as68.68k -l -u %1.s
..\m68 lo68.68k -r -u_nofloat -o %1.68k s.o %1.o clib
del %1.o

