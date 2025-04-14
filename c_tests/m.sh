#!/bin/bash
#set -x

gccpath=~/outrun/gcc-8.2.0
gcccmd=$gccpath/bin/m68k-elf-gcc


inc1=$gccpath/lib/gcc/m68k-elf/8.2.0/include
inc2=$gccpath/m68k-elf/include
inc3=..
incpaths="-I$inc1 -I$inc2 -I$inc3"

#lib1=$gccpath/m68k-elf/lib
#lib2=$gccpath/lib/gcc/m68k-elf/8.2.0
#libpaths="-L$lib1 -L$lib2"

# generate .s files for debugging
$gcccmd $incpaths -mcpu=68000 -x c++ -O2 $1.c -S -fverbose-asm -o $1.s
$gcccmd $incpaths -mcpu=68000 -x c++ $incpaths -O2 newlib68.c -S -fverbose-asm -o newlib68.s

# build the assembly portion with _start and syscalls
$gccpath/bin/m68k-elf-as -mcpu=68000 m68start.s -o m68start.o

$gcccmd $incpaths -mcpu=68000 -x c++ -O2 $1.c newlib68.c -l:m68start.o -L./ -static-libgcc -l:libm.a -l:libstdc++.a -static -o $1.elf


