@echo off
setlocal

set outputfile=test_elfto68k.txt
echo %date% %time% >%outputfile%

if "%1" == "nested" (
  set _M68runcmd=..\m68 -h:80 ..\c_tests\m68.elf
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

set _clist=hidave tprintf tm tmuldiv ttt sieve e tstr targs tbits t tao ^
           tcmp ttypes tarray trw mm_old fileops tpi ^
           t_setjmp td tf tap tphi mm ts glob nantst pis tfo ^
           fopentst lenum tex

( for %%a in (%_clist%) do ( call :crun %%a ) )

goto :alldone

:crun

echo building %~1
echo building %~1 >>%outputfile%
call mt.bat %~1 >>%outputfile%
echo running %~1
echo running %~1 >>%outputfile%
%_M68runcmd% %~1.68K >>%outputfile%
exit /b 0

:alldone

echo %date% %time% >>%outputfile%
diff baseline_%outputfile% %outputfile%

:eof

