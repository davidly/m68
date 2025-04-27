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

%_runit% cp68.68k -i 0: %1.c %1.i
%_runit% c068.68k %1.i %1.1 %1.2 %1.3 -f
del  %1.i 2>nul
%_runit% c168.68k %1.1 %1.2 %1.s
del %1.1 2>nul
del %1.2 2>nul
%_runit% as68.68k -l -u -s 0: %1.s
rem del %1.s 2>nul

