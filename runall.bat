@echo off
setlocal

set _runcmd=m68
set _M68runcmd=..\%_runcmd%

if "%1" == "nested" (
  set _runcmd=m68 -h:80 c_tests\m68.elf
  set _M68runcmd=..\%_runcmd%
)

if "%1" == "armos" (
  set _runcmd=..\armos\armos -h:80 ..\armos\bin\m68
  set _M68runcmd=..\%_runcmd%
)

if "%1" == "rvos" (
  set _runcmd=..\rvos\rvos -h:80 ..\rvos\linux\m68
  set _M68runcmd=..\%_runcmd%
)

set outputfile=test_m68.txt
echo %date% %time% >%outputfile%

rem note that tmuldiv results are incorrect due to compiler bugs but consistent with m68k

set _applist=sieve e ttt tm ts tpi tmuldiv tstr mm tprintf tshift

( for %%a in (%_applist%) do ( call :appRun %%a ) )

rem many tests including nantst produce different results than other compiler/ISA implementations.
rem for example, the old gcc for m68k has a different value for infinity for floating point numbers.

set _elflist=hidave tprintf tm tmuldiv ttt sieve e tstr targs tbits t tao ^
             tcmp ttypes tarray trw terrno mm_old ttime fileops tpi ^
             t_setjmp td tf tap tphi mm ts glob nantst pis tfo sleeptm ^
             tbcd tshift taddsubm tea ttt68 ttt68u

( for %%a in (%_elflist%) do ( call :elfRun %%a ) )

set _compList=cpm mtpascal cb68 cpmcv11 cpmcv12 syspas sysfor

( for %%a in (%_compList%) do ( call :compRun %%a ) )

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

echo test m68.elf ttt.elf 1
echo test m68.elf ttt.elf 1 >>%outputfile%
%_runcmd% -h:60 c_tests\m68.elf c_tests\ttt.elf 1 >>%outputfile%

echo test m68.elf ttt.68k 1
echo test m68.elf ttt.68k 1 >>%outputfile%
%_runcmd% -h:60 c_tests\m68.elf cpm\ttt.68k 1 >>%outputfile%

echo test com cp/m 2.2 emulator
echo test com cp/m 2.2 emulator >>%outputfile%
pushd com
%_M68runcmd% -h:60 COM.68K MBASIC.COM HELLO.BAS >>..\%outputfile%
popd

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

:compRun

echo compiler test %~1
echo compiler test %~1 >>%outputfile%
pushd %~1
call mall.bat >>..\%outputfile%
call runall.bat >>..\%outputfile%
popd
exit /b 0

:alldone

echo %date% %time% >>%outputfile%
diff baseline_%outputfile% %outputfile%

:eof



