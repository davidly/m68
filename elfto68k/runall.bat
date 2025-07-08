@echo off
setlocal

set outputfile=test_elfto68k.txt
echo %date% %time% >%outputfile%

if "%1" == "nested" (
  set _M68runcmd=..\m68 -h:80 ..\c_tests\m68.elf
)

if "%1" == "nested68k" (
  set _M68runcmd=..\m68 -h:80 m68.68k
)

if "%1" == "armos" (
  set _M68runcmd=..\..\armos\armos -h:80 ..\..\armos\bin\m68
)

if "%1" == "rvos" (
  set _M68runcmd=..\..\rvos\rvos -h:80 ..\..\rvos\linux\m68
)

if "%_M68runcmd%" == "" (
  set _M68runcmd=..\m68
)

echo building elfto68k
echo building elfto68k >>%outputfile%
call m.bat >>%outputfile%

echo building m68
echo building m68 >>%outputfile%
call mm68.bat >>%outputfile%

set _clist=hidave tprintf tm tmuldiv ttt sieve e tstr targs tbits t tao ^
           tcmp ttypes tarray trw trw2 mm_old fileops tpi ^
           t_setjmp td tf tap tphi mm ts glob nantst pis tfo ^
           fopentst lenum tex trename

( for %%a in (%_clist%) do ( call :crun %%a ) )

echo running m68.68k
echo running m68.68k >>%outputfile%
%_M68runcmd% M68.68K -h:4 trw.68k >>%outputfile%

echo building ba
echo building ba >>%outputfile%
copy ..\c_tests\ba.c . >nul
call mt.bat ba >>%outputfile%
echo running ba
echo running ba >>%outputfile%
%_M68runcmd% ba.68K TP.BAS >>%outputfile%

echo building an
echo building an >>%outputfile%
copy ..\c_tests\an.c . >nul
call mt.bat an >>%outputfile%
echo running an
echo running an >>%outputfile%
%_M68runcmd% AN.68K david lee >>%outputfile%

goto :alldone

:crun

echo building %~1
echo building %~1 >>%outputfile%
copy ..\c_tests\%~1.c . >nul
call mt.bat %~1 >>%outputfile%
echo running %~1
echo running %~1 >>%outputfile%
%_M68runcmd% %~1.68K >>%outputfile%
exit /b 0

:alldone

echo %date% %time% >>%outputfile%
diff baseline_%outputfile% %outputfile%

:eof

