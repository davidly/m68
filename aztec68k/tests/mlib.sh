#!/bin/bash
#set -x

# compile the portion written in C here

ntvdm -u -r:.. -e:PATH=C:\\BIN\\ ../BIN/C68.EXE -IC:\\INCLUDE -o AZCPM.R AZCPM.C

# start with the mac library since it's similar to what's needed. remove modules that are truly mac-specific or stubs

cp ../LIB/C68MAC.LIB C68K.LIB
ntvdm ../BIN/LB68.EXE C68K.LIB -d mac
ntvdm ../BIN/LB68.EXE C68K.LIB -d open close unlink lseek read isatty rename
ntvdm ../BIN/LB68.EXE C68K.LIB -r write AZCPM.R
ntvdm ../BIN/LB68.EXE C68K.LIB -d pbwrite

# replace the version of format that doesn't understand floating point with one that does

cp C68K.LIB C68KF.LIB
ntvdm ../BIN/LB68.EXE C68KF.LIB -r format ..\\LIB\\MISC\\FFORMAT.R

# assemble the piece with the cp/m 68k startup and support code. m.bat/mf.bat link with cpm.r and c68k.lib/c68kf.lib

ntvdm -r:.. -u -c ../BIN/AS68 CPM.ASM -o CPM.R
