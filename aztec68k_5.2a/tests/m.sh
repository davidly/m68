#!/bin/bash
#set -x

if [ "$1" == "" ]; then
    echo "Usage: m.sh <sourcefile>"
    exit 1
fi

str=$(tr '[a-z]' '[A-Z]' <<< $1)

# note: if you haven't run mlib.sh yet, do that first. that creates objects this script relies on.

# the Aztec C compiler for 68k is an MS-DOS cross-compiler. Use ntvdm or your favorate DOS emulator to run the tools.
# https://github.com/davidly/ntvdm

# first compile the app

ntvdm -u -r:.. -e:PATH=C:\\BIN\\ ../BIN/C68.EXE -IC:\\INCLUDE -o $str.R $str.C

# Code starts at 0x8100 so the base page is at 0x8000.
# -lm drags in floating point code. if you don't need that, leave it out

ntvdm -u -r:.. -e:CLIB68=C:\\LIB\\LIBS\;PATH=C:\\BIN\\ ../BIN/LN68.EXE +C 8100 -t CPM.R AZCPM.R AZMALLOC.R $str.R -lm -lC68K -T -O $str.68K

# copy symbols from app.sym to app.68k (the linker doesn't do this)

symto68k $str

