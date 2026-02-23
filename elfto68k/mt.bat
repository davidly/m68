@echo off
setlocal

if "%1" == "" (goto noargs)

rem building with -O2 yields 33% faster test runtimes (because compiling is faster) and smaller binaries for CP/M 68k machines
if "%2" == "" (set _optflag=2) else (set _optflag=%2)

if "%3" == "" (set gccver=13.2.0) else (set gccver=%3)

set gccpath=..\gcc-%gccver%
path=%gccpath%\bin;%path%

set inc1=%gccpath%\lib\gcc\m68k-elf\%gccver%\include
set inc2=%gccpath%\m68k-elf\include
set inc3=..\
set inc4=.
set inc5=.\bits
set includes=-I%inc4% -I%inc5% -I%inc1% -I%inc2% -I%inc3%

set gcc=%gccpath%\bin\m68k-elf-gcc

rem The linker makes the address space start at 0x2000 lower than the address of .init. Close enough.
rem The 68008-based SBC I have that runs CP/M 68K requires apps to be loaded pretty high compared to
rem machines from the 1980's. Set to load around 32k.
set ldflags=-Wl,--section-start=.init=0xa000 -Wl,--gc-sections

rem WIN_GCC_HANG is defined for the Windows version of gcc 13.2.0, which hangs compiling some normal C code in ttypes.c
set gccflags=-mcpu=68000 -x c++ -fexceptions -fno-use-cxa-atexit -O%_optflag% -DWIN_GCC_HANG -ffunction-sections -fdata-sections

rem generate .s files for debugging
rem %gcc% %includes% %gccflags% %1.c -S -fverbose-asm -o %1.s
rem %gcc% %includes% %gccflags% nlcpm.c -S -fverbose-asm -o nlcpm.s

rem build the assembly portion with _start and syscalls
%gccpath%\bin\m68k-elf-as -mcpu=68000 startcpm.s -o startcpm.o

rem actually compile and link the app

%gcc% %defines% %includes% %gccflags% %1.c nlcpm.c -l:startcpm.o -L./ -static-libgcc -l:libm.a -l:libstdc++.a -static -o %1.elf %ldflags%

if %ERRORLEVEL% NEQ 0 ( goto alldone )

elfto68k %1.elf
del %1.elf

goto :eof

:noargs
echo no source file specified!


