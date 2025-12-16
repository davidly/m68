@echo off
setlocal

set gccpath=c:\outrun\gcc-8.2.0
path=%gccpath%\bin;%path%

%gccpath%\bin\m68k-elf-gcc --help
%gccpath%\bin\m68k-elf-gcc --help=optimizers




