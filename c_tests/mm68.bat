@echo off
setlocal

set _optflag=3

set gccpath=..\gcc-8.2.0
path=%gccpath%\bin;%path%

set inc1=%gccpath%\lib\gcc\m68k-elf\8.2.0\include
set inc2=%gccpath%\m68k-elf\include
set inc3=..\
set inc4=..\..\djl
set inc5=%gccpath%\m68k-elf\include\c++\8.2.0\bits
set includes=-I%inc1% -I%inc2% -I%inc3% -I%inc4% -I%inc5%

set gcc=%gccpath%\bin\m68k-elf-gcc

rem M68 means we're building the m68.elf binary (vs another emulator). M68K means we're using the M68K GCC compiler targeting 68000
set defines=-DTARGET_BIG_ENDIAN -DM68 -DM68K -DNDEBUG
set gccflags=-mcpu=68000 -x c++ -fno-use-cxa-atexit

rem generate .s files for debugging
%gcc% %defines% %includes% %gccflags% -O%_optflag% ..\m68.cxx -S -fverbose-asm -o m68.s
%gcc% %defines% %includes% %gccflags% -O%_optflag% ..\m68000.cxx -S -fverbose-asm -o m68000.s
%gcc% %defines% %includes% %gccflags% -O%_optflag% newlib68.c -S -o newlib68.s

rem build the assembly portion with _start and syscalls
%gccpath%\bin\m68k-elf-as -mcpu=68000 m68start.s -o m68start.o

rem actually build the app
%gcc% %defines% %includes% %gccflags% -O%_optflag% ..\m68.cxx ..\m68000.cxx newlib68.c -l:m68start.o -L./ -l:libm.a -l:libstdc++.a -o m68.elf


