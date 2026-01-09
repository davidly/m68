@echo off
setlocal

if "%1" == "" (goto noargs)

rem note: if you haven't run mlib.bat yet, do that first. that creates objects this script relies on.

rem the Aztec C compiler for 68k is an MS-DOS cross-compiler. Use ntvdm to run the tools.

rem first compile the app

ntvdm -r:.. -e:PATH=C:\BIN\ ..\bin\c68 -Ic:\include -o %1.r %1.c

rem Code starts at 0x8100 so the base page is at 0x8000.
rem c68k.lib is the Mac library c68mac.lib with mac, write, and pbwrite removed using lb68.exe. see mlib.bat.
rem That mac-specific functionality is ported to cp/m 68k in cpm.asm and linked first below.
rem This is the 16-bit int version of the libraries. cpm.asm would need changes to support 32-bit ints.
rem Other than writes to stdout, no file i/o is implemented in the C runtime. elfto68k\nlcpm.c is a starting point for such work.
rem m68.lib is linked for floating point support in mf.bat. use c68kf.lib with floating point support.

ntvdm -r:.. -e:CLIB68=C:\LIB\;PATH=C:\BIN\ ..\bin\ln68 +C 8100 -t cpm.r %1.r -lm68 -lc68kf -T -O %1.68k

rem copy symbols from app.sym to app.68k (the linker doesn't do this)

symto68k %1

goto :eof

:noargs
echo no source file specified!

