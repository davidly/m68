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

set _clist=e sieve ttt tm fileops tpi t_setjmp tmuldiv mm

( for %%a in (%_clist%) do ( call :crun %%a ) )

goto :makeasm

:crun

echo building %~1
call m.bat %~1
exit /b 0

:makeasm

set _asmlist=tchk tmovep tea tbcd taddsubm tshift

( for %%a in (%_asmlist%) do ( call :asmRun %%a ) )

set _justasmlist=ttt68u e68 sieve68

( for %%a in (%_justasmlist%) do ( call :justasmRun %%a ) )

goto :alldone

:asmRun

echo building %~1
call ma.bat %~1
exit /b 0

:justasmRun

echo building %~1
call maa.bat %~1
exit /b 0

:alldone

