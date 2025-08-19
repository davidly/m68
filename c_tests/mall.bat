@echo off
setlocal

if "%1" == "" (set _optflag=2) else (set _optflag=%1)

set _elflist=hidave tprintf tm tmuldiv ttt sieve e tstr targs tbits t tao ^
             tcmp ttypes tarray trw trw2 an terrno mm_old ttime fileops tpi ^
             t_setjmp ba td tf tap tphi mm ts glob nantst pis tfo sleeptm ff ^
             nqueens nq1d tdir fopentst lenum tgets tex trename

( for %%a in (%_elflist%) do (
    echo building %%a
    call m.bat %%a %_optflag%
) )

:makem68elf
echo build m68.elf
call mm68.bat

set _asmlist=tbcd tshift taddsubm tea ttt68 ttt68u tchk

( for %%a in (%_asmlist%) do (
    echo building %%a
    call ma.bat %%a
) )

:alldone

