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

set _intlist=e sieve ttt mm nqueens nq1d
( for %%a in (%_intlist%) do (
    echo building %%a
    call m.bat %%a
))

set _floatlist=tphi tap
( for %%a in (%_floatlist%) do (
    echo building %%a
    call mf.bat %%a
))
