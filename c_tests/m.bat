@echo off
setlocal

if "%1" == "" (goto noargs)

if "%2" == "" (set _optflag=2) else (set _optflag=%2)

if "%3" == "" (set gccver=8.2.0) else (set gccver=%3)
rem if "%3" == "" (set gccver=13.2.0) else (set gccver=%3)

set gccpath=..\gcc-%gccver%

path=%gccpath%\bin;%path%

set inc1=%gccpath%\lib\gcc\m68k-elf\%gccver%\include
set inc2=%gccpath%\m68k-elf\include
set inc3=.\
set inc4=..\..\djl
set inc5=%gccpath%\m68k-elf\include\c++\%gccver%\bits
set inc6=..\

set includes=-I%inc1% -I%inc2% -I%inc3% -I%inc4% -I%inc5% -I%inc6%

set gcc=%gccpath%\bin\m68k-elf-gcc

rem the linker makes the address space start at 0x2000 lower than the address of .init. Close enough
set ldflags=-Wl,--section-start=.init=0x4000

rem WIN_GCC_HANG is defined for the Windows version of gcc 13.2.0, which hangs compiling some normal C code in ttypes.c
set defines=-DNDEBUG -DWIN_GCC_HANG
set gccflags=-mcpu=68000 -x c++ -fno-use-cxa-atexit -O%_optflag%

rem generate .s files for debugging
rem %gcc% %includes% %gccflags% %1.c -S -fverbose-asm -o %1.s
rem %gcc% %includes% %gccflags% newlib68.c -S -fverbose-asm -o newlib68.s

rem build the assembly portion with _start and syscalls
%gccpath%\bin\m68k-elf-as -mcpu=68000 m68start.s -o m68start.o

rem actually build the app
%gcc% %defines% %includes% %gccflags% %1.c newlib68.c -l:m68start.o -L./ -l:libm.a -l:libstdc++.a -o %1.elf %ldflags% 
%gccpath%\m68k-elf\bin\objdump.exe -d --insn-width=4 %1.elf >%1.txt

goto alldone

:noargs
echo no source file specified!

:alldone

