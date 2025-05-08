@echo off
setlocal

if "%1" == "nested" (
  set _M68runcmd=..\m68 -h:80 ..\c_tests\m68.elf
)

if "%1" == "armos" (
  set _M68runcmd=..\..\armos\armos -h:80 ..\..\armos\bin\m68
)

if "%1" == "rvos" (
  set _M68runcmd=..\..\rvos\rvos -h:80 ..\..\rvos\linux\m68
)

if "%_M68runcmd%" == "" (
  set _M68runcmd=..\m68
)

set _clist=E SIEVE TTT TM TTT68U T_SETJMP TCHK TMOVEP MM

( for %%a in (%_clist%) do ( call :crun %%a ) )

goto :alldone

:crun

echo running %~1
rem the C runtime zeroes bss, so use a small bss
%_M68runcmd% -h:1 %~1.68k
exit /b 0

:alldone


