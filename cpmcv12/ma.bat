@echo off
setlocal

if "%_M68runcmd%" == "" (
  set _M68runcmd=..\m68
)

if "%1" == "" (
  echo no file specified
  goto eof
)

%_M68runcmd% -h:1 as68.68k -l -u %1.s

%_M68runcmd% -h:1 lo68.68k -r -u_nofloat -o %1.68k s.o %1.o clib

del %1.o

:eof

