#!/bin/bash
#set -x

if [ "$1" == "" ]; then
    optflags="2"
else
    optflags="$1"
fi

if [ "$2" == "" ]; then
    gccver="13.2.0"
else
    gccver=$2
fi

gccpath=../gcc-$gccver-linux
gcccmd=$gccpath/bin/m68k-elf-gcc

inc1=$gccpath/lib/gcc/m68k-elf/$gccver/include
inc2=$gccpath/m68k-elf/include
inc3=..
incpaths="-I. -I./bits -I$inc1 -I$inc2 -I$inc3"

# M68 means we're building the m68.elf binary (vs another emulator).
defines="-DTARGET_BIG_ENDIAN -DM68 -DNDEBUG"
gccflags="-mcpu=68000 -x c++ -fno-use-cxa-atexit -O$optflags -ffunction-sections -fdata-sections"
ldflags="-Wl,--section-start=.init=0x4000 -Wl,--gc-sections"

# generate .s files for debugging
#$gcccmd $incpaths $gccflags $defines ../m68.cxx -S -fverbose-asm -o m68.s
#$gcccmd $incpaths $gccflags $defines ../m68000.cxx -S -fverbose-asm -o m68000.s
#$gcccmd $incpaths $gccflags $defines nlcpm.c -S -fverbose-asm -o nlcpm.s

# build the assembly portion with _start and syscalls
$gccpath/bin/m68k-elf-as -mcpu=68000 startcpm.s -o startcpm.o

$gcccmd $defines $incpaths $gccflags ../m68.cxx ../m68000.cxx nlcpm.c -l:startcpm.o -L./ -static-libgcc -l:libm.a -l:libstdc++.a -static -o m68.elf $ldflags

elfto68k m68.elf
rm m68.elf

