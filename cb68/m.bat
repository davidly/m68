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

%_runit% cb68.68k %1.bas

rem cb68 doesn't appear to set an exit code
rem if %errorlevel% neq 0 (
rem   echo basic compiler mt68 exit code: %errorlevel%
rem   goto eof
rem )

%_runit% link68.68k %1.o,cb68.l68

if %errorlevel% neq 0 (
  echo linker link68 exit code: %errorlevel%
  goto eof
)

del %1.o 2>nul

:eof

