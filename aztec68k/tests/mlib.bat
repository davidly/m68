@echo off
setlocal

rem compile the portion written in C here

ntvdm -r:.. -e:PATH=C:\BIN\ ..\bin\c68 -Ic:\include -o azcpm.r azcpm.c

rem start with the mac library since it's similar to what's needed. remove modules that are truly mac-specific or stubs

copy ..\lib\c68mac.lib c68k.lib > NUL
ntvdm ..\bin\lb68.exe c68k.lib -d mac
ntvdm ..\bin\lb68.exe c68k.lib -d open close unlink lseek read isatty rename
ntvdm ..\bin\lb68.exe c68k.lib -r write azcpm.r
ntvdm ..\bin\lb68.exe c68k.lib -d pbwrite

rem replace the version of format that doesn't understand floating point with one that does

copy c68k.lib C68KF.LIB > NUL
ntvdm ..\bin\lb68.exe c68kf.lib -r format ..\lib\misc\fformat.r

rem assemble the piece with the cp/m 68k startup and support code. m.bat/mf.bat link with cpm.r and c68k.lib/c68kf.lib

ntvdm -c ..\bin\as68 cpm.asm -o cpm.r

