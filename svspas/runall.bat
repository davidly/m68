@echo off
setlocal

if "%1" == "nested" (
  set _M68runcmd=..\m68 -h:80 ..\c_tests\m68.elf -h:60
)

if "%1" == "armos" (
  set _M68runcmd=..\..\armos\armos -h:80 ..\..\armos\bin\m68 -h:60
)

if "%1" == "rvos" (
  set _M68runcmd=..\..\rvos\rvos -h:80 ..\..\rvos\linux\m68 -h:60
)

if "%_M68runcmd%" == "" (
  set _M68runcmd=..\m68
)

set _clist=E SIEVE TTT MM

( for %%a in (%_clist%) do ( call :crun %%a ) )

goto :alldone

:crun

echo running %~1
%_M68runcmd% %~1.68k
exit /b 0

:alldone

