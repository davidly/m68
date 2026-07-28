@echo off
setlocal

rem compile the portion written in C here

ntvdm -r:.. -e:PATH=C:\BIN\ ..\bin\c68 -Ic:\include -o AZCPM.R azcpm.c
ntvdm -r:.. -e:PATH=C:\BIN\ ..\bin\c68 -Ic:\include -o AZMALLOC.R azmalloc.c

rem start with the mac library since it's similar to what's needed. remove modules that are truly mac-specific or stubs

copy ..\lib\libs\c.lib C68K.LIB > NUL
ntvdm ..\bin\lb68.exe C68K.LIB -d open _open close _close unlink lseek _lseek read _read isatty rename write _write pbwrite exit errno _unlink
ntvdm ..\bin\lb68.exe C68K.LIB -d malloc calloc realloc free

rem assemble the piece with the cp/m 68k startup and support code. m.bat/mf.bat link with cpm.r and c68k.lib/c68kf.lib

ntvdm -c ..\bin\as68 cpm.asm -o CPM.R

