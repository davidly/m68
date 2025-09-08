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

set _clist=e sieve ttt tm fileops tpi t_setjmp tmuldiv

( for %%a in (%_clist%) do (
    echo building %%a
    call m.bat %%a
))

set _asmlist=ttt68u tchk tmovep

( for %%a in (%_asmlist%) do (
    echo building %%a
    call ma.bat %%a
))
