#!/bin/bash
#set -x

if [ "$1" == "" ]; then
    echo "Usage: m.sh <sourcefile> [optlevel] [gccpath]"
    echo "  optlevel: 0, 1, 2 (default), 3, fast"
    echo "  gccver: version of gcc toolchain (e.g. 8.2.0 for ../gcc-8.2.0-linux)"
    exit 1
fi

if [ "$2" == "" ]; then
    optflags="2"
else
    optflags="$2"
fi

if [ "$3" == "" ]; then
    gccver="8.2.0"
else
    gccver=$3
fi

gccpath=../gcc-$gccver-linux
gcccmd=$gccpath/bin/m68k-elf-gcc

inc1=$gccpath/lib/gcc/m68k-elf/$gccver/include
inc2=$gccpath/m68k-elf/include
inc3=..
incpaths="-I. -I./bits -I$inc1 -I$inc2 -I$inc3"

ldflags="-Wl,--section-start=.init=0x4000"
gccflags="-mcpu=68000 -x c++ -fexceptions -fno-use-cxa-atexit -O$optflags"

# generate .s files for debugging
#$gcccmd $incpaths $gccflags $1.c -S -fverbose-asm -o $1.s
#$gcccmd $incpaths $gccflags newlib68.c -S -fverbose-asm -o newlib68.s

# build the assembly portion with _start and syscalls
$gccpath/bin/m68k-elf-as -mcpu=68000 m68start.s -o m68start.o

$gcccmd $incpaths $gccflags $1.c newlib68.c -l:m68start.o -L./ -static-libgcc -l:libm.a -l:libstdc++.a -static -o $1.elf $ldflags


