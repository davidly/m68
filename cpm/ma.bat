@echo off

..\m68 as68.68k -l -u -s 0: %1.s
..\m68 lo68.68k -r -u_nofloat -o %1.68k 0:s.o %1.o 0:clib



