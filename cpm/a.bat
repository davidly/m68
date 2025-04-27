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

%_runit% as68.68k -l -u -s 0: %1.s

:eof


