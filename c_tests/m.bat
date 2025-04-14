@echo off
setlocal

if "%1" == "" (goto noargs)

rem O3 and Ofast have at least 3 bugs in code generation that I've found
if "%2" == "" (set _optflag=2) else (set _optflag=%2)

set gccpath=..\gcc-8.2.0
path=%gccpath%\bin;%path%

set inc1=%gccpath%\lib\gcc\m68k-elf\8.2.0\include
set inc2=%gccpath%\m68k-elf\include
set inc3=..\
set includes=-I%inc1% -I%inc2% -I%inc3%

set lib1=%gccpath%\m68k-elf\lib
set lib2=%gccpath%\lib\gcc\m68k-elf\8.2.0
set libpaths=-L%lib1% -L%lib2%

set defines=-D__SINGLE_THREAD___

set gcc=%gccpath%\bin\m68k-elf-gcc

rem generate .s files for debugging
%gcc% %includes% -mcpu=68000 -x c++ -O%_optflag% %1.c -S -fverbose-asm -o %1.s
%gcc% %includes% -mcpu=68000 -x c++ -O%_optflag% newlib68.c -S -fverbose-asm -o newlib68.s

rem build the assembly portion with _start and syscalls
%gccpath%\bin\m68k-elf-as -mcpu=68000 m68start.s -o m68start.o

rem actually build the app
%gcc% %defines% %includes% -mcpu=68000 -x c++ -O%_optflag% %1.c newlib68.c -l:m68start.o -L./ -static-libgcc -l:libm.a %libpaths% -l:libstdc++.a -static -o %1.elf

goto alldone

:noargs
echo no source file specified!

:alldone

