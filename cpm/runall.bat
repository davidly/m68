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

set _clist=E SIEVE TTT TM FILEOPS TPI TTT68U T_SETJMP TMULDIV TCHK TMOVEP

( for %%a in (%_clist%) do ( call :crun %%a ) )

rem compile and run a basic app
%_M68runcmd% ..\c_tests\ba -x -a:c ..\c_tests\tp.bas
copy ..\c_tests\tp.s
call ma.bat tp
%_M68runcmd% tp.68k

goto :alldone

:crun

echo running %~1
%_M68runcmd% %~1.68k
exit /b 0

:alldone


