@echo off
setlocal

if "%M68Nested%" == "" (
  set _runit=..\m68
) ELSE (
  set _runit=..\m68 -h:60 ..\c_tests\m68.elf
)

set _clist=E SIEVE TTT

( for %%a in (%_clist%) do ( call :crun %%a ) )

goto :alldone

:crun

echo running %~1
%_runit% %~1.68k
exit /b 0

:alldone

