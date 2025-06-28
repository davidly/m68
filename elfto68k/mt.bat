rem @echo off
setlocal

if "%1" == "" (goto noargs)

if "%2" == "" (set _optflag=2) else (set _optflag=%2)

set gccpath=..\gcc-8.2.0
path=%gccpath%\bin;%path%

set inc1=%gccpath%\lib\gcc\m68k-elf\8.2.0\include
set inc2=%gccpath%\m68k-elf\include
set inc3=..\
set inc4=.
set inc5=.\bits
set includes=-I%inc4% -I%inc5% -I%inc1% -I%inc2% -I%inc3%

set gcc=%gccpath%\bin\m68k-elf-gcc

rem The linker makes the address space start at 0x2000 lower than the address of .init. Close enough.
rem The 68008-based SBC I have that runs CP/M 68K requires apps to be loaded pretty high compared to
rem machines from the 1980's. Set to load around 32k.
set ldflags=-Wl,--section-start=.init=0xa000

set gccflags=-mcpu=68000 -x c++ -fexceptions -fno-use-cxa-atexit -O%_optflag%

rem generate .s files for debugging
rem %gcc% %includes% %gccflags% %1.c -S -fverbose-asm -o %1.s
rem %gcc% %includes% %gccflags% nlcpm.c -S -fverbose-asm -o nlcpm.s

rem build the assembly portion with _start and syscalls
%gccpath%\bin\m68k-elf-as -mcpu=68000 startcpm.s -o startcpm.o

rem actually compile and link the app

%gcc% %defines% %includes% %gccflags% %1.c nlcpm.c -l:startcpm.o -L./ -static-libgcc -l:libm.a -l:libstdc++.a -static -o %1.elf %ldflags%

if %ERRORLEVEL% NEQ 0 ( goto alldone )

elfto68k %1.elf

goto alldone

:noargs
echo no source file specified!

:alldone

