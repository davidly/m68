@echo off
setlocal

set _clist=e sieve ttt

( for %%a in (%_clist%) do ( call :crun %%a ) )

goto :alldone

:crun

echo building %~1
call m.bat %~1
exit /b 0

:alldone

