@echo off
setlocal

rem build an assembly app that doesn't link with the C runtime.

if "%_M68runcmd%" == "" (
  set _M68runcmd=..\m68
)

if "%1" == "" (
  echo no file specified
  goto eof
)

del %1.68K 2>nul
%_M68runcmd% as68.68k -l -u %1.s

%_M68runcmd% lo68.68k -x -r -o %1.68k %1.o

del %1.o 2>nul

:eof

