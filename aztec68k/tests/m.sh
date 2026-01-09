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
# c68k.lib is the Mac library c68mac.lib with mac, write, and pbwrite removed using lb68.exe. see mlib.bat.
# That mac-specific functionality is ported to cp/m 68k in cpm.asm and linked first below.
# This is the 16-bit int version of the libraries. cpm.asm would need changes to support 32-bit ints.
# Other than writes to stdout, no file i/o is implemented in the C runtime. elfto68k\nlcpm.c is a starting point for such work.
# m68.lib is linked for floating point support in mf.bat, not here.

ntvdm -u -r:.. -e:CLIB68=C:\\LIB\\\;PATH=C:\\BIN\\ ../BIN/LN68.EXE +C 8100 -t CPM.R $str.R -lC68K -T -O $str.68K

# copy symbols from app.sym to app.68k (the linker doesn't do this)

symto68k $str

