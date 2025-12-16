@echo off
setlocal

rem set gccpath=..\gcc-8.2.0
set gccpath=..\gcc-13.2.0
path=%gccpath%\bin;%path%

%gccpath%\bin\m68k-elf-gcc -E -dM hidave.c

%gccpath%\bin\m68k-elf-gcc --version


