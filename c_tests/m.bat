@echo off
setlocal

if "%1" == "" (goto noargs)

if "%2" == "" (set _optflag=2) else (set _optflag=%2)

if "%3" == "" (set gccpath=..\gcc-8.2.0) else (set gccpath=%3)

path=%gccpath%\bin;%path%

set inc1=%gccpath%\lib\gcc\m68k-elf\8.2.0\include
set inc2=%gccpath%\m68k-elf\include
set inc3=..\
set inc4=.
set inc5=.\bits
set includes=-I%inc4% -I%inc5% -I%inc1% -I%inc2% -I%inc3%

set gcc=%gccpath%\bin\m68k-elf-gcc

rem the linker makes the address space start at 0x2000 lower than the address of .init. Close enough
set ldflags=-Wl,--section-start=.init=0x4000

set gccflags=-mcpu=68000 -x c++ -fexceptions -fno-use-cxa-atexit -O%_optflag%

rem generate .s files for debugging
rem %gcc% %includes% %gccflags% %1.c -S -fverbose-asm -o %1.s
rem %gcc% %includes% %gccflags% newlib68.c -S -fverbose-asm -o newlib68.s

rem build the assembly portion with _start and syscalls
%gccpath%\bin\m68k-elf-as -mcpu=68000 m68start.s -o m68start.o

rem actually build the app

rem standard way
rem %gcc% %defines% %includes% %gccflags% %1.c newlib68.c -l:m68start.o -L./ -static-libgcc -l:libm.a -l:libstdc++.a -static -o %1.elf

rem using a linker script
rem %gcc% %defines% %includes% %gccflags% %1.c newlib68.c -l:m68start.o -L./ -static-libgcc -l:libm.a -l:libstdc++.a -static -o %1.elf -Wl,-Tlscript.txt

%gcc% %defines% %includes% %gccflags% %1.c newlib68.c -l:m68start.o -L./ -static-libgcc -l:libm.a -l:libstdc++.a -static -o %1.elf %ldflags% 

goto alldone

:noargs
echo no source file specified!

:alldone

