@echo off
setlocal

if "%1" == "" (set _optflag=3) else (set _optflag=%1)

if "%2" == "" (set gccver=13.2.0) else (set gccver=%2)

set gccpath=..\gcc-%gccver%

path=%gccpath%\bin;%path%

set inc1=%gccpath%\lib\gcc\m68k-elf\%gccver%\include
set inc2=%gccpath%\m68k-elf\include
set inc3=..\
set inc4=..\..\djl
set includes=-I. -I.\bits -I%inc1% -I%inc2% -I%inc3% -I%inc4%

set gcc=%gccpath%\bin\m68k-elf-gcc
set ldflags=-Wl,--section-start=.init=0x4000 -Wl,--gc-sections

rem M68 means we're building the m68.elf binary (vs another emulator).
set defines=-DTARGET_BIG_ENDIAN -DM68 -DNDEBUG
set gccflags=-mcpu=68000 -x c++ -fno-use-cxa-atexit -O%_optflag% -ffunction-sections -fdata-sections

rem generate .s files for debugging
%gcc% %defines% %includes% %gccflags% ..\m68.cxx -S -fverbose-asm -o m68.s
%gcc% %defines% %includes% %gccflags% ..\m68000.cxx -S -fverbose-asm -o m68000.s
%gcc% %defines% %includes% %gccflags% newlib68.c -S -o newlib68.s

rem build the assembly portion with _start and syscalls
%gccpath%\bin\m68k-elf-as -mcpu=68000 m68start.s -o m68start.o

rem actually build the app
%gcc% %defines% %includes% %gccflags% ..\m68.cxx ..\m68000.cxx newlib68.c -l:m68start.o -L./ -l:libm.a -l:libstdc++.a -static -o m68.elf %ldflags%

%gccpath%\m68k-elf\bin\objdump.exe -d m68.elf >m68.txt

copy m68.elf ..\bin 1>nul 2>nul

