#!/bin/bash
#set -x

# compile the portions written in C here

ntvdm -u -r:.. -e:PATH=C:\\BIN\\ ../BIN/C68.EXE -IC:\\INCLUDE -o AZCPM.R AZCPM.C
ntvdm -u -r:.. -e:PATH=C:\\BIN\\ ../BIN/C68.EXE -IC:\\INCLUDE -o AZMALLOC.R AZMALLOC.C

# Start with the standard C library. Remove modules replaced by the CP/M
# support code or by the custom allocator.

cp ../LIB/LIBS/C.LIB C68K.LIB
ntvdm ../BIN/LB68.EXE C68K.LIB -d open _open close _close unlink lseek _lseek read _read isatty rename write _write pbwrite exit errno _unlink
ntvdm ../BIN/LB68.EXE C68K.LIB -d malloc calloc realloc free

# Assemble the CP/M-68K startup and support code.
# m.sh links CPM.R, AZCPM.R, AZMALLOC.R, M.LIB, and C68K.LIB.

ntvdm -r:.. -u -c ../BIN/AS68 CPM.ASM -o CPM.R
