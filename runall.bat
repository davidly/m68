@echo off
setlocal

set _runcmd=m68 -h:60
if "%1" == "nested" (set _runcmd=m68cl -h:80 c_tests\m68.elf -h:60 )

set outputfile=test_m68.txt
echo %date% %time% >%outputfile%

rem note that tmuldiv results are incorrect due to compiler bugs but consistent with m68k

set _applist=sieve e ttt tm ts tpi tmuldiv tstr mm tprintf tshift

( for %%a in (%_applist%) do ( call :appRun %%a ) )

rem many tests including nantst produce different results than other compiler/ISA implementations.
rem for example, the old gcc for m68k has a different value for infinity for floating point numbers.

set _elflist=hidave tprintf tm tmuldiv ttt sieve e tstr targs tbits t tao ^
             tcmp ttypes tarray trw terrno mm_old ttime fileops tpi ^
             t_setjmp td tf tap tphi mm ts glob nantst pis ^
             tbcd tshift taddsubm tea ttt68 ttt68u

( for %%a in (%_elflist%) do ( call :elfRun %%a ) )

set _cpmlist=ttt68u ttt e sieve tm

( for %%a in (%_cpmlist%) do ( call :cpmRun %%a ) )

rem 1-off tests

echo test an -t david lee
echo test an -t david lee >>%outputfile%
%_runcmd% c_tests\an -t david lee >>%outputfile%

echo test ba tp.bas
echo test ba tp.bas >>%outputfile%
%_runcmd% c_tests\ba c_tests\tp.bas >>%outputfile%

set _genlist=6 8 a d 3 i I m o r x
( for %%g in (%_genlist%) do ( call :genRun %%g ) )

goto after_ba

:genRun

%_runcmd% c_tests\ba -a:%~1 -x c_tests\tp.bas >>%outputfile%
exit /b 0

:after_ba

echo test m68.elf ttt 1
echo test m68.elf ttt 1 >>%outputfile%
%_runcmd% c_tests\m68.elf c_tests\ttt 1 >>%outputfile%

goto :alldone

:appRun

echo test %~1.hex
echo test %~1.hex >>%outputfile%
%_runcmd% hexapps\%~1.hex >>%outputfile%
exit /b 0

:elfRun

echo test %~1
echo test %~1 >>%outputfile%
%_runcmd% c_tests\%~1 >>%outputfile%
exit /b 0

:cpmRun

echo test %~1.68k
echo test %~1.68k >>%outputfile%
%_runcmd% cpm\%~1.68k >>%outputfile%
exit /b 0

:alldone

echo %date% %time% >>%outputfile%
diff baseline_%outputfile% %outputfile%

:eof



