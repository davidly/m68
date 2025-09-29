@echo off
setlocal

if "%1" == "" (goto noargs)

if "%2" == "" (set _optflag=2) else (set _optflag=%2)

if "%3" == "" (set gccver=8.2.0) else (set gccver=%3)

set gccpath=..\gcc-%gccver%
path=%gccpath%\bin;%path%

set inc1=%gccpath%\lib\gcc\m68k-elf\%gccver%\include
set inc2=%gccpath%\m68k-elf\include
set inc3=..\
set inc4=.
set inc5=.\bits
set includes=-I%inc4% -I%inc5% -I%inc1% -I%inc2% -I%inc3%

set ldflags=-Wl,--section-start=.init=0x4000
set gcc=%gccpath%\bin\m68k-elf-gcc
set gccflags=-mcpu=68000 -x c++ -fexceptions -fno-use-cxa-atexit -O%_optflag%

rem build the assembly portion with _start and syscalls
%gccpath%\bin\m68k-elf-as -mcpu=68000 m68start.s -o m68start.o

rem build the actual app
%gccpath%\bin\m68k-elf-as -mcpu=68000 %1.s -o %1.o

rem actually build the app
%gcc% %defines% %includes% %gccflags% newlib68.c -l:%1.o -l:m68start.o -L./ -static-libgcc -l:libm.a -l:libstdc++.a -static -o %1.elf %ldflags%

goto alldone

:noargs
echo no source file specified!

:alldone


