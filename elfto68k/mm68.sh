#!/bin/bash
#set -x

gccpath=~/outrun/gcc-8.2.0
gcccmd=$gccpath/bin/m68k-elf-gcc

inc1=$gccpath/lib/gcc/m68k-elf/8.2.0/include
inc2=$gccpath/m68k-elf/include
inc3=..
incpaths="-I. -I./bits -I$inc1 -I$inc2 -I$inc3"

# M68 means we're building the m68.elf binary (vs another emulator). M68K means we're using the M68K GCC compiler to target 68000
defines="-DTARGET_BIG_ENDIAN -DM68 -DM68K -DNDEBUG"
gccflags="-mcpu=68000 -x c++ -fno-use-cxa-atexit -O3"
ldflags="-Wl,--section-start=.init=0x4000"

# generate .s files for debugging
$gcccmd $incpaths $gccflags $defines ../m68.cxx -S -fverbose-asm -o m68.s
$gcccmd $incpaths $gccflags $defines ../m68000.cxx -S -fverbose-asm -o m68000.s
$gcccmd $incpaths $gccflags $defines nlcpm.c -S -fverbose-asm -o nlcpm.s

# build the assembly portion with _start and syscalls
$gccpath/bin/m68k-elf-as -mcpu=68000 startcpm.s -o startcpm.o

$gcccmd $defines $incpaths $gccflags ../m68.cxx ../m68000.cxx nlcpm.c -l:startcpm.o -L./ -static-libgcc -l:libm.a -l:libstdc++.a -static -o m68.elf $ldflags

elfto68k m68.elf
rm m68.elf

