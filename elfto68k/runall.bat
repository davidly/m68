@echo off
setlocal

set outputfile=test_elfto68k.txt
echo %date% %time% >%outputfile%

if "%1" == "nested" (
    set _M68rutdncmd=..\m68 -h:80 ..\c_tests\m68.elf
    shift
)

if "%1" == "nested68k" (
    set _M68runcmd=..\m68 -h:80 m68.68k
    shift
)

if "%1" == "armos" (
    set _M68runcmd=..\..\armos\armos -h:80 ..\..\armos\bin\m68
    shift
)

if "%1" == "rvos" (
    set _M68runcmd=..\..\rvos\rvos -h:80 ..\..\rvos\linux\m68
    shift
)

if "%1" == "sparcos" (
    set _M68runcmd=..\..\sparcos\sparcos -h:80 ..\..\sparcos\bin\m68-sparc.elf
    shift
)

if "%_M68runcmd%" == "normal" (
    set _M68runcmd=..\m68
    shift
)

if "%_M68runcmd%" == "" (
    set _M68runcmd=..\m68
)

if "%1" == "" (set _optflag=2) else (set _optflag=%1)

rem note that ttypes.c produces difference results for gcc 8.2.0 vs gcc 13.2.0
if "%2" == "" (set _gccfolder=..\gcc-8.2.0) else (set _gccfolder=%2)

echo building elfto68k
echo building elfto68k >>%outputfile%
call m.bat >>%outputfile%

echo building m68
echo building m68 >>%outputfile%
call mm68.bat %_optflag% %_gccfolder% >>%outputfile%

set _clist=hidave tprintf tm tmuldiv ttt sieve e tstr targs tbits t tao ^
           tcmp ttypes tarray trw trw2 terrno mm_old fileops tpi ^
           t_setjmp td tf tap tphi mm ts glob nantst pis tfo ^
           nqueens nq1d fopentst lenum tex trename

( for %%a in (%_clist%) do (
    echo building %%a
    echo building %%a >>%outputfile%
    copy ..\c_tests\%%a.c . >nul
    call mt.bat %%a %_optflag% %_gccfolder% >>%outputfile%
    echo running %%a
    echo running %%a >>%outputfile%
    %_M68runcmd% %%a.68K >>%outputfile%
    
    if "%%a" == "trw" (
        echo running trw in m68.68k
        echo running trw in m68.68k >>%outputfile%
        %_M68runcmd% M68.68K -h:4 trw.68k >>%outputfile%
    )
    
    del %%a.s 2>nul
    del %%a.c 2>nul
    del %%a.68k 2>nul
) )

echo building ba
echo building ba >>%outputfile%
copy ..\c_tests\ba.c . >nul
call mt.bat ba %_optflag% %_gccfolder% >>%outputfile%
echo running ba
echo running ba >>%outputfile%
%_M68runcmd% ba.68K TP.BAS >>%outputfile%
del ba.s 2>nul
del ba.c 2>nul
del ba.68k 2>nul

echo building an
echo building an >>%outputfile%
copy ..\c_tests\an.c . >nul
call mt.bat an %_optflag% %_gccfolder% >>%outputfile%
echo running an
echo running an >>%outputfile%
%_M68runcmd% AN.68K david lee >>%outputfile%
del an.s 2>nul
del an.c 2>nul
del an.68k 2>nul

echo building ff
echo building ff >>%outputfile%
copy ..\c_tests\ff.c . >nul
copy ..\c_tests\realpath.c . >nul
copy ..\c_tests\fnmatch.c . >nul
call mt.bat ff %_optflag% %_gccfolder% >>%outputfile%
echo running ff
echo running ff >>%outputfile%
%_M68runcmd% FF.68K -i M*.68K >>%outputfile%
del ff.s 2>nul
del ff.c 2>nul
del realpath.s 2>nul
del realpath.c 2>nul
del fnmatch.s 2>nul
del fnmatch.c 2>nul
del ff.68k 2>nul

echo %date% %time% >>%outputfile%
diff baseline_%outputfile% %outputfile%

