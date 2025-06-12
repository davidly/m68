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

%_M68runcmd% MT68.68K %1.pas

%_M68runcmd% link68.68k [symbols] %1,fpreals.l68,paslib.l68

if %errorlevel% neq 0 (
  echo linker link68 exit code: %errorlevel%
  goto eof
)

del %1.err 2>nul
del %1.o 2>nul

:eof

