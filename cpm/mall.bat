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

set _clist=e sieve ttt tm fileops tpi t_setjmp tmuldiv mm fopentst tgets ^
           nqueens nq1d cpmenumd primes targs

( for %%a in (%_clist%) do (
    echo building %%a
    call m.bat %%a
))

set _cflist=floata mandle tap tphi ts tf pis

( for %%a in (%_cflist%) do (
    echo building floating point app %%a
    call mf.bat %%a
))

set _asmlist=tchk tmovep tea tbcd taddsubm tshift

( for %%a in (%_asmlist%) do (
    echo building %%a
    call ma.bat %%a
))

set _justasmlist=ttt68u e68 sieve68 nq68 nq681d

( for %%a in (%_justasmlist%) do (
    echo building %%a
    call maa.bat %%a
))


