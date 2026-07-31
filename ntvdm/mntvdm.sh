#!/bin/sh

set -e

cp ../c_tests/newlib68.c .
cp ../c_tests/m68start.s .

optflag=3

gccpath="../gcc-8.2.0-linux"

# Add toolchain to PATH
export PATH="$gccpath/bin:$PATH"

inc1="$gccpath/lib/gcc/m68k-elf/8.2.0/include"
inc2="$gccpath/m68k-elf/include"
inc3="../c_tests"
inc4="../../ntvdm"
inc5="$gccpath/m68k-elf/include/c++/8.2.0/bits"
inc6="../"

includes="
-I$inc1
-I$inc2
-I$inc3
-I$inc4
-I$inc5
-I$inc6
"

gcc="$gccpath/bin/m68k-elf-gcc"
as="$gccpath/bin/m68k-elf-as"

defines="
-DTARGET_BIG_ENDIAN
-DNDEBUG
"

gccflags="
-mcpu=68000
-x c++
-fno-use-cxa-atexit
"

echo "Generating assembly listings..."
"$gcc" \
    $defines \
    $includes \
    $gccflags \
    -O"$optflag" \
    ntvdm.cxx \
    -S \
    -fverbose-asm \
    -o ntvdm.s

"$gcc" \
    $defines \
    $includes \
    $gccflags \
    -O"$optflag" \
    i8086.cxx \
    -S \
    -fverbose-asm \
    -o i8086.s

echo "Building C runtime stub..."
"$gcc" \
    $defines \
    $includes \
    $gccflags \
    -O"$optflag" \
    newlib68.c \
    -S \
    -o newlib68.s

echo "Assembling startup..."
"$as" \
    -mcpu=68000 \
    m68start.s \
    -o m68start.o

echo "Building executable..."
"$gcc" \
    $defines \
    $includes \
    $gccflags \
    -O"$optflag" \
    ntvdm.cxx \
    i8086.cxx \
    newlib68.c \
    -Wl,-l:m68start.o \
    -L./ \
    -Wl,-l:libm.a \
    -Wl,-l:libstdc++.a \
    -o ntvdm.elf

cp ntvdm.elf ../bin/ntvdm

echo "Done."