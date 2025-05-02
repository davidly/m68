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

del %1.68k 2>nul
%_runit% cp68.68k %1.c %1.i

%_runit% c068.68k %1.i %1.1 %1.2 %1.3 -f

del %1.i 2>nul
%_runit% c168.68k %1.1 %1.2 %1.s

del %1.1 2>nul
del %1.2 2>nul
%_runit% as68.68k -l -u %1.s

del %1.s 2>nul

%_runit% lo68.68k -r -o %1.68k s.o %1.o clib libf.a 

del %1.o 2>nul

:eof

