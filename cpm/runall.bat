@echo off
setlocal

if "%1" == "nested" (
  set _M68runcmd=..\m68 -h:100 ..\c_tests\m68.elf
)

if "%1" == "armos" (
  set _M68runcmd=..\..\armos\armos -h:10 ..\..\armos\bin\m68
)

if "%1" == "rvos" (
  set _M68runcmd=..\..\rvos\rvos -h:100 ..\..\rvos\bin\m68
)

if "%1" == "x64os" (
  set _M68runcmd=..\..\x64os\x64os -h:100 ..\..\x64os\bin\m68
)

if "%1" == "x32os" (
  set _M68runcmd=..\..\x64os\x32os -h:100 ..\..\x64os\x32bin\m68
)

if "%1" == "sparcos" (
  set _M68runcmd=..\..\sparcos\sparcos -h:100 ..\..\sparcos\bin\m68-sparc.elf
)

if "%_M68runcmd%" == "" (
  set _M68runcmd=..\m68
)

rem compile and run a basic app
%_M68runcmd% ..\c_tests\ba -x -a:c -q ..\c_tests\tp.bas
copy ..\c_tests\tp.s
call ma.bat tp
%_M68runcmd% tp.68k

set _clist=E SIEVE TTT TM FILEOPS TPI TTT68U T_SETJMP TMULDIV TCHK TMOVEP MM ^
           E68 SIEVE68 TEA TBCD TADDSUBM TSHIFT FOPENTST NQUEENS NQ1D CPMENUMD FLOATA ^
           MANDLE TAP TPHI TS PRIMES TF PIS TARGS NQ68 NQ681D TRW TRW2 TBIG

( for %%a in (%_clist%) do (
    echo running %%a
    %_M68runcmd% %%a.68k
))

rem run app with redirected input
echo running tgets with redirected stdin
%_M68runcmd% tgets.68k <tgets.txt


