@echo off
setlocal

copy ..\c_tests\newlib68.c
copy ..\c_tests\m68start.s

set _optflag=3

set gccpath=..\gcc-8.2.0
path=%gccpath%\bin;%path%

set inc1=%gccpath%\lib\gcc\m68k-elf\8.2.0\include
set inc2=%gccpath%\m68k-elf\include
set inc3=..\c_tests
set inc4=..\..\djl
set inc5=%gccpath%\m68k-elf\include\c++\8.2.0\bits
set inc6=..\

set includes=-I%inc1% -I%inc2% -I%inc3% -I%inc4% -I%inc5% -I%inc6%

set gcc=%gccpath%\bin\m68k-elf-gcc

set defines=-DTARGET_BIG_ENDIAN -DNDEBUG
set gccflags=-mcpu=68000 -x c++ -fno-use-cxa-atexit

rem generate .s files for debugging
%gcc% %defines% %includes% %gccflags% -O%_optflag% ntvao.cxx -S -fverbose-asm -o ntvao.s
%gcc% %defines% %includes% %gccflags% -O%_optflag% mos6502.cxx -S -fverbose-asm -o mos6502.s

rem build the C runtime stubs .s file
%gcc% %defines% %includes% %gccflags% -O%_optflag% newlib68.c -S -o newlib68.s

rem build the assembly portion with _start and syscalls
%gccpath%\bin\m68k-elf-as -mcpu=68000 m68start.s -o m68start.o

rem actually build the app
%gcc% %defines% %includes% %gccflags% -O%_optflag% ntvao.cxx mos6502.cxx newlib68.c -l:m68start.o -L./ -l:libm.a -l:libstdc++.a -o ntvao.elf

copy ntvao.elf ..\bin\ntvao

