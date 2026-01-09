@echo off
if "%1" == "" set PATH=C:\WINDOWS\system32;C:\WINDOWS
set CROOT=%1
if "%1" == "" set CROOT=C:\AZTEC68K
set CLIB68=%CROOT%\LIB\
set INCL68=%CROOT%\INCLUDE
set CR65=%CROOT%\R\
set PATH=%CROOT%\BIN;%CROOT%;%PATH%;
set TEMP=C:\WINDOWS\TEMP
SET CCTEMP=%TEMP%
echo Aztec C68K 3.6b Rom 6-6-88 
echo Copyright 1982-88 Manx Software Systems, Inc.
