@echo off
setlocal

if "%_M68runcmd%" == "" (
  set _M68runcmd=..\m68
)

if "%1" == "" (
  echo no file specified
  goto eof
)

del %1.68k 2>nul
%_M68runcmd% -h:1 cp68.68k %1.c %1.i

%_M68runcmd% -h:1 c068.68k %1.i %1.1 %1.2 %1.3 -f

del %1.i 2>nul
%_M68runcmd% -h:1 c168.68k %1.1 %1.2 %1.s

del %1.1 2>nul
del %1.2 2>nul

%_M68runcmd% -h:1 as68.68k -l -u %1.s

del %1.s

%_M68runcmd% -h:1 lo68.68k -r -u_nofloat -o %1.68k s.o %1.o clib

del %1.o

:eof

