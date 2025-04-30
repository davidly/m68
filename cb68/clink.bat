@echo off

setlocal

if "%M68Nested%" == "" (
  set _runit=..\m68
) ELSE (
  set _runit=..\m68 -h:80 ..\c_tests\m68.elf
)

if "%1" == "" (
  echo no file specified
  goto eof
)

%_runit% lo68.68k -r -u_nofloat -o %1.68k 0:s.o %1.o %2.o %3.o %4.o %5.o %6.o %7.o %8.o %9.o 0:clib

:eof


