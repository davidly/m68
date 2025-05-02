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
%_M68runcmd% -h:1 cp.68k %1.c %1.i

%_M68runcmd% -h:1 c0.68k %1.i %1.ic %1.st

del %1.i 2>nul
%_M68runcmd% -h:1 c1.68k %1.ic %1.s -l

del %1.ic 2>nul
del %1.st 2>nul

%_M68runcmd% -h:1 as.68k -l -u %1.s

del %1.s

%_M68runcmd% -h:1 lo.68k -r -o %1.68k s.o %1.o clib

del %1.o

:eof

