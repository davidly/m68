@echo off
setlocal

rem separate tests are in elfto68k and aztec68k\tests runall scripts

if "%1" == "" (
    set _runcmd=m68
    set _M68runcmd=..\m68
) else (
if "%1" == "nested" (
    set _runcmd=m68 -h:180 c_tests\m68.elf
    set _M68runcmd=..\m68 -h:180 ..\c_tests\m68.elf
) else (
if "%1" == "armos" (
    set _runcmd=..\armos\armos -h:180 ..\armos\bin\m68
    set _M68runcmd=..\..\armos\armos -h:180 ..\..\armos\bin\m68
) else (
if "%1" == "rvos" (
    set _runcmd=..\rvos\rvos -h:180 ..\rvos\bin\m68
    set _M68runcmd=..\..\rvos\rvos -h:180 ..\..\rvos\bin\m68
) else (
if "%1" == "x64os" (
    set _runcmd=..\x64os\x64os -h:180 ..\x64os\bin\m68
    set _M68runcmd=..\..\x64os\x64os -h:180 ..\..\x64os\bin\m68
) else (
if "%1" == "x32os" (
    set _runcmd=..\x64os\x32os -h:180 ..\x64os\x32bin\m68
    set _M68runcmd=..\..\x64os\x32os -h:180 ..\..\x64os\x32bin\m68
) else (
if "%1" == "sparcos" (
    set _runcmd=..\sparcos\sparcos -h:180 ..\sparcos\bin\m68-sparc
    set _M68runcmd=..\..\sparcos\sparcos -h:180 ..\..\sparcos\bin\m68-sparc
) else (
    echo invalid argument
    goto :eof
)))))))

set outputfile=test_m68.txt
echo %date% %time% >%outputfile%

rem note that tmuldiv results are incorrect due to compiler bugs but consistent with m68k

set _hexlist=sieve e ttt tm ts tpi tmuldiv tstr mm tprintf tshift

( for %%a in (%_hexlist%) do (
    echo test %%a.hex
    echo test %%a.hex >>%outputfile%
    %_runcmd% hexapps\%%a.hex >>%outputfile%
))

rem many tests including nantst produce different results than other compiler/ISA implementations.
rem for example, the old gcc for m68k has a different value for infinity for floating point numbers.

set _elflist=hidave tprintf tm tmuldiv ttt sieve e tstr targs tbits t tao ^
             tcmp ttypes tarray trw trw2 terrno mm_old ttime fileops tpi ^
             t_setjmp td tf tap tphi mm ts glob nantst pis tfo sleeptm ^
             nqueens nq1d tdir fopentst lenum tex trename tmmap termiosf ^
             tbcd tshift taddsubm tea ttt68 ttt68u tchk tsh

( for %%a in (%_elflist%) do (
    echo test %%a
    echo test %%a>>%outputfile%
    %_runcmd% c_tests\%%a >>%outputfile%
))

set _compList=cpm mtpascal cb68 cpmcv11 cpmcv12 svspas svsfor forth83

( for %%a in (%_compList%) do (
    echo compiler test %%a
    echo compiler test %%a >>%outputfile%
    pushd %%a
    call mall.bat >>..\%outputfile%
    call runall.bat >>..\%outputfile%
    popd
))

rem 1-off tests

echo test an -t david lee
echo test an -t david lee >>%outputfile%
%_runcmd% c_tests\an -t david lee >>%outputfile%

echo test ba tp.bas
echo test ba tp.bas >>%outputfile%
%_runcmd% c_tests\ba -q c_tests\tp.bas >>%outputfile%

set _genlist=6 8 a d 3 i I m o r x
( for %%g in (%_genlist%) do (
    %_runcmd% c_tests\ba -a:%%g -x -q c_tests\tp.bas >>%outputfile%
))

echo test m68.elf ttt.elf 1
echo test m68.elf ttt.elf 1 >>%outputfile%
%_runcmd% -h:120 c_tests\m68.elf c_tests\ttt.elf 1 >>%outputfile%

echo test m68.elf ttt.68k 1
echo test m68.elf ttt.68k 1 >>%outputfile%
%_runcmd% -h:120 c_tests\m68.elf cpm\ttt.68k 1 >>%outputfile%

echo test com cp/m 2.2 emulator
echo test com cp/m 2.2 emulator >>%outputfile%
pushd com
%_M68runcmd% -h:1 COM.68K MBASIC.COM HELLO.BAS >>..\%outputfile%
popd

echo running tgets with redirected stdin
echo running tgets with redirected stdin >>%outputfile%
%_runcmd% c_tests\tgets <tgets.txt >>%outputfile%

echo test ff . ff.c
echo test ff . ff.c >>%outputfile%
%_runcmd% c_tests\ff . ff.c >>%outputfile%

echo test targs
echo test targs a bb ccc dddd >>%outputfile%
%_runcmd% c_tests\targs a bb ccc dddd >>%outputfile%

echo %date% %time% >>%outputfile%
dos2unix -f %outputfile%
diff -b baseline_%outputfile% %outputfile%

:eof

