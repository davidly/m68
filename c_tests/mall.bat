@echo off
setlocal

if "%1" == "" (set _optflag=2) else (set _optflag=%1)

if "%2" == "" (set _gccver=8.2.0) else (set _gccver=%2)

set _gccfolder=..\gcc-%_gccver%

if not exist %_gccfolder% (
    echo "gcc folder doesn't exist: " %_gccver%
    echo "sample usage: mall 2 13.2.0"
    echo "  optimization level and folder where gcc resides"
    goto alldone
)

set _elflist=hidave tprintf tm tmuldiv ttt sieve e tstr targs tbits t tao ^
             tcmp ttypes tarray trw trw2 an terrno mm_old ttime fileops tpi ^
             t_setjmp ba td tf tap tphi mm ts glob nantst pis tfo sleeptm ff ^
             nqueens nq1d tdir fopentst lenum tgets tex trename tmmap tenv

( for %%a in (%_elflist%) do (
    echo building %%a
    call m.bat %%a %_optflag% %_gccver%
) )

:makem68elf
echo build m68.elf
rem hard-code optimization level to 3
call mm68.bat 3 %_gccver%

set _asmlist=tbcd tshift taddsubm tea ttt68 ttt68u tchk

( for %%a in (%_asmlist%) do (
    echo building %%a
    call ma.bat %%a %_optflag% %_gccver%
) )

:alldone

