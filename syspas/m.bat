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

%_M68runcmd% -h:1 -h:1 pascal.68k %1.pas
%_M68runcmd% -h:1 -h:1 code.68k %1.i
%_M68runcmd% -h:1 -h:1 ulinker.68k -l %1.o %1.obj ftnlib.obj paslib.obj
del %1.obj
%_M68runcmd% -h:1 -h:1 lo68.68k -r -o %1.68k -u__optoff s.o %1.o clib
del %1.o


