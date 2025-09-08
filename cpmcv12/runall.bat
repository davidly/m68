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

if "%1" == "sparcos" (
  set _M68runcmd=..\..\sparcos\sparcos -h:80 ..\..\sparcos\bin\m68-sparc.elf
)

if "%_M68runcmd%" == "" (
  set _M68runcmd=..\m68
)

set _applist=E SIEVE TTT TM FILEOPS TPI TTT68U T_SETJMP TMULDIV TCHK TMOVEP

( for %%a in (%_applist%) do (
    echo running %%a
    %_M68runcmd% %%a.68k
))



