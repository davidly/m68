@echo off
setlocal

if "%1" == "" (set _optflag=2) else (set _optflag=%1)

set _elflist=hidave tprintf tm tmuldiv ttt sieve e tstr targs tbits t tao ^
             tcmp ttypes tarray trw an terrno mm_old ttime fileops tpi ^
             t_setjmp ba td tf tap tphi mm ts glob nantst pis tfo sleeptm

( for %%a in (%_elflist%) do ( call :elfRun %%a ) )

goto :makem68elf

:elfRun

echo building %~1
call m.bat %~1 %_optflag%
exit /b 0

:makem68elf
echo build m68.elf
call mm68.bat

set _asmlist=tbcd tshift taddsubm tea ttt68 ttt68u

( for %%a in (%_asmlist%) do ( call :asmRun %%a ) )

goto :alldone

:asmRun

echo building %~1
call ma.bat %~1
exit /b 0

:alldone

