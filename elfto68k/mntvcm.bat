@echo off
setlocal

set gccpath=..\gcc-13.2.0
path=%gccpath%\bin;%path%

set inc1=%gccpath%\lib\gcc\m68k-elf\13.2.0\include
set inc2=%gccpath%\m68k-elf\include
set inc3=..\
set inc4=..\..\djl
set inc5=%gccpath%\m68k-elf\include\c++\13.2.0\bits
set inc6=..\ntvcm
set includes=-I. -I.\bits -I%inc1% -I%inc2% -I%inc3% -I%inc4% -I%inc5% -I%inc6%

set gcc=%gccpath%\bin\m68k-elf-gcc
set ldflags=-Wl,--section-start=.init=0xa000

rem M68 means we're building the m68.elf binary (vs another emulator).
set defines=-DTARGET_BIG_ENDIAN -DM68 -DNDEBUG
set gccflags=-mcpu=68000 -x c++ -fno-use-cxa-atexit -O3

rem generate .s files for debugging
rem %gcc% %defines% %includes% %gccflags% ..\ntvcm\ntvcm.cxx -S -fverbose-asm -o ntvcm.s
rem %gcc% %defines% %includes% %gccflags% ..\ntvcm\x80.cxx -S -fverbose-asm -o x80.s
rem %gcc% %defines% %includes% %gccflags% nlcpm.c -S -o nlcpm.s

rem build the assembly portion with _start and syscalls
%gccpath%\bin\m68k-elf-as -mcpu=68000 startcpm.s -o startcpm.o

rem actually build the app
%gcc% %defines% %includes% %gccflags% ..\ntvcm\ntvcm.cxx ..\ntvcm\x80.cxx nlcpm.c -l:startcpm.o -L./ -l:libm.a -l:libstdc++.a -static -o ntvcm.elf %ldflags%

if %ERRORLEVEL% NEQ 0 ( goto alldone )

elfto68k ntvcm.elf
del ntvcm.elf

:alldone

