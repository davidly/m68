#!/bin/bash
#set -x

gccpath=~/outrun/gcc-8.2.0
gcccmd=$gccpath/bin/m68k-elf-gcc

inc1=$gccpath/lib/gcc/m68k-elf/8.2.0/include
inc2=$gccpath/m68k-elf/include
inc3=..
incpaths="-I$inc1 -I$inc2 -I$inc3"

# build the assembly portion with _start and syscalls
$gccpath/bin/m68k-elf-as -mcpu=68000 m68start.s -o m68start.o

# build the app
$gccpath/bin/m68k-elf-as -mcpu=68000 $1.s -o $1.o

$gcccmd $incpaths -mcpu=68000 -x c++ -O2 newlib68.c -l:m68start.o -l:$1.o -L./ -static-libgcc -l:libm.a -l:libstdc++.a -static -o $1.elf


