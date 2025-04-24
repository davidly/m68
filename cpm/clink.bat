@echo off

..\m68 lo68.68k -r -u_nofloat -o %1.68k 0:s.o %1.o %2.o %3.o %4.o %5.o %6.o %7.o %8.o %9.o 0:clib

