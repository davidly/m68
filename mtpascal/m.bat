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

%_runit% MT68.68K %1.pas

rem the pascal compiler doesn't emit an exit code
rem if %errorlevel% neq 0 (
rem   echo pascal compiler mt68 exit code: %errorlevel%
rem   goto eof
rem )

%_runit% link68.68k %1,paslib.l68

if %errorlevel% neq 0 (
  echo linker link68 exit code: %errorlevel%
  goto eof
)

del %1.err 2>nul
del %1.o 2>nul

:eof

