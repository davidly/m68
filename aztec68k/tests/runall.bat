@echo off
setlocal

set _runcmd=..\..\m68

set outputfile=test_aztec.txt
echo %date% %time% >%outputfile%

rem build libraries
call mlib.bat >>%outputfile%

rem build symto68k utility
call msymto68k.bat >>%outputfile%

rem tests without floating point build with m.bat
set _clist=SIEVE E TTT TM TARGS HI TPI MM NQ1D PRIMES NQUEENS T_SETJMP FOPENTST TRW TRW2 FILEOPS TRENAME TBDOS TPRINTF TBIG

( for %%a in (%_clist%) do (
    echo test %%a
    echo test %%a >>%outputfile%
    call m.bat %%a >>%outputfile%
    %_runcmd% %%a.68k >>%outputfile%
    del %%a.r
    del %%a.sym
    del %%a.68k
))

rem tests with floating point build with mf.bat
set _flist=TPHI TAP TMULDIV PIS MANDLE TS TF

( for %%a in (%_flist%) do (
    echo test %%a
    echo test %%a >>%outputfile%
    call mf.bat %%a >>%outputfile%
    %_runcmd% %%a.68k >>%outputfile%
    del %%a.r
    del %%a.sym
    del %%a.68k
))

echo %date% %time% >>%outputfile%
dos2unix %outputfile%
diff -b baseline_%outputfile% %outputfile%

:eof

