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

set _intlist=e sieve ttt mm
( for %%a in (%_intlist%) do ( call :intrun %%a ) )

set _floatlist=tphi
( for %%a in (%_floatlist%) do ( call :floatrun %%a ) )

goto :alldone

:intrun

echo building %~1
call m.bat %~1
exit /b 0

:floatrun

echo building %~1
call mf.bat %~1
exit /b 0

:alldone

