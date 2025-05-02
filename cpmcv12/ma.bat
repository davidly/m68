@echo off
setlocal

if "%_M68runcmd%" == "" (
  set _M68runcmd=..\m68
)

if "%1" == "" (
  echo no file specified
  goto eof
)

del %1.68K 2>nul
%_M68runcmd% as68.68k -l -u %1.s

rem it doesn't appear the assembler sets an exit code
if %errorlevel% neq 0 (
  echo assembler as68 exit code: %errorlevel%
  goto eof
)

%_M68runcmd% lo68.68k -r -u_nofloat -o %1.68k s.o %1.o clib

if %errorlevel% neq 0 (
  echo linker lo68 exit code: %errorlevel%
  goto eof
)

del %1.o 2>nul

:eof

