@echo off
setlocal

copy ..\c_tests\newlib68.c
copy ..\c_tests\m68start.s

rem set gccver=8.2.0
set gccver=13.2.0

set gccpath=..\gcc-%gccver%
path=%gccpath%\bin;%path%

set inc1=%gccpath%\lib\gcc\m68k-elf\%gccver%\include
set inc2=%gccpath%\m68k-elf\include
set inc3=..\c_tests
set inc4=..\..\djl
set inc5=%gccpath%\m68k-elf\include\c++\%gccver%\bits
set inc6=..\

set includes=-I%inc1% -I%inc2% -I%inc3% -I%inc4% -I%inc5% -I%inc6%

rem the linker makes the address space start at 0x2000 lower than the address of .init. Close enough
set ldflags=-Wl,--section-start=.init=0x4000 -Wl,--gc-sections

set gcc=%gccpath%\bin\m68k-elf-gcc

set defines=-DTARGET_BIG_ENDIAN -DNDEBUG -DRVOS
set gccflags=-mcpu=68000 -x c++ -fno-use-cxa-atexit -O3 -ffunction-sections -fdata-sections

rem generate .s files for debugging
%gcc% %defines% %includes% %gccflags% rvos.cxx -S -fverbose-asm -o rvos.s
%gcc% %defines% %includes% %gccflags% riscv.cxx -S -fverbose-asm -o riscv.s

rem build the C runtime stubs .s file
%gcc% %defines% %includes% %gccflags% newlib68.c -S -o newlib68.s

rem build the assembly portion with _start and syscalls
%gccpath%\bin\m68k-elf-as -mcpu=68000 m68start.s -o m68start.o

rem actually build the app
%gcc% %defines% %includes% %gccflags% rvos.cxx riscv.cxx newlib68.c -l:m68start.o -L./ -l:libm.a -l:libstdc++.a -o rvos.elf %ldflags%
%gccpath%\m68k-elf\bin\objdump.exe -d rvos.elf >rvos.txt

copy rvos.elf ..\bin\rvos

