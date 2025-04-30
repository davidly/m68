@echo off
setlocal

set _clist=e sieve ttt tm fileops tpi t_setjmp tmuldiv

( for %%a in (%_clist%) do ( call :crun %%a ) )

goto :makeasm

:crun

echo building %~1
call m.bat %~1
exit /b 0

:makeasm

set _asmlist=ttt68u tchk

( for %%a in (%_asmlist%) do ( call :asmRun %%a ) )

goto :alldone

:asmRun

echo building %~1
call ma.bat %~1
exit /b 0

:alldone

