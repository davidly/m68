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

rem it appears tmuldiv and fileops fail due to corruption in t C runtime file object. tpi won't link
set _clist=e sieve ttt tm t_setjmp mm

( for %%a in (%_clist%) do ( call :crun %%a ) )

goto :makeasm

:crun

echo building %~1
call m.bat %~1
exit /b 0

:makeasm

set _asmlist=ttt68u tchk tmovep

( for %%a in (%_asmlist%) do ( call :asmRun %%a ) )

goto :alldone

:asmRun

echo building %~1
call ma.bat %~1
exit /b 0

:alldone

