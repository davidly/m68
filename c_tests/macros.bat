@echo off
setlocal

set gccpath=..\gcc-8.2.0
path=%gccpath%\bin;%path%

set gcc=%gccpath%\bin\m68k-elf-gcc

echo | %gcc% -dM -E -


