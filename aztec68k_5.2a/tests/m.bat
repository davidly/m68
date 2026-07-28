@echo off
setlocal

if "%1" == "" (goto noargs)

rem note: if you haven't run mlib.bat yet, do that first. that creates objects this script relies on.

rem the Aztec C compiler for 68k is an MS-DOS cross-compiler. Use ntvdm or your favorate DOS emulator to run the tools.
rem https://github.com/davidly/ntvdm

rem first compile the app

ntvdm -r:.. -e:PATH=C:\BIN\ ..\bin\c68 -Ic:\include -o %1.r %1.c

rem Code starts at 0x8100 so the base page is at 0x8000.
rem -lm drags in floating point code. if you don't need that, leave it out

ntvdm -r:.. -e:CLIB68=C:\LIB\LIBS;PATH=C:\BIN\ ..\bin\ln68 +C 8100 -t cpm.r azcpm.r azmalloc.r %1.r -lm -lC68K -T -O %1.68K

rem copy symbols from app.sym to app.68k (the linker doesn't do this)

symto68k %1

goto :eof

:noargs
echo no source file specified!

