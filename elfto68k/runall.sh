#!/bin/bash
#set -x

if [ "$1" = "nested" ]; then
    _m68runcmd="../m68 -h:180 ../c_tests/m68.elf"
fi

if [ "$1" = "nested68k" ]; then
    _m68runcmd="../m68 -h:180 M68.68K"
fi

if [ "$1" = "armos" ]; then
    _m68runcmd="../../ArmOS/armos -h:180 ../../ArmOS/bin/m68"
fi

if [ "$1" = "sparcos" ]; then
    _m68runcmd="../../sparcos/sparcos -h:180 ../../sparcos/bin/m68"
fi

if [ "$1" = "rvos" ]; then
    _m68runcmd="../../rvos/rvos -h:180 ../../rvos/bin/m68.elf"
fi

if [ "$1" = "normal" ]; then
    _m68runcmd="../m68"
fi

if [ "$_m68runcmd" = "" ]; then
    _m68runcmd="../m68"
fi

if [ "$2" == "" ]; then
    optflags="2"
else
    optflags=$2
fi

if [ "$3" == "" ]; then
    gccpath="../gcc-8.2.0-linux"
else
    gccpath=$3
fi

outputfile="test_elfto68k.txt"
date_time=$(date)
echo "$date_time" >$outputfile

mkdir sys >/dev/null 2>&1
cp ../c_tests/sys/* sys >/dev/null 2>&1

echo building elfto68k
echo building elfto68k >>$outputfile
m.sh >>$outputfile

echo building m68
echo building m68 >>$outputfile
mm68.sh $optflags $gccpath >>$outputfile

for arg in hidave tprintf tm tmuldiv ttt sieve e tstr targs tbits t tao \
           tcmp ttypes tarray trw trw2 terrno mm_old fileops tpi \
           t_setjmp td tf tap tphi mm ts glob nantst pis tfo \
           nqueens nq1d fopentst lenum tex trename
do
  echo building $arg
  echo building $arg >>$outputfile
  cp ../c_tests/$arg.c . >/dev/null
  mt.sh $arg $optflags $gccpath >>$outputfile
  echo running $arg
  echo running $arg >>$outputfile
  argu=$(tr '[a-z]' '[A-Z]' <<< $arg)
  $_m68runcmd $argu.68K >>$outputfile

  if [ "$argu" = "TRW" ]; then
    echo running trw in m68.68k
    echo running trw in m68.68k >>$outputfile
    $_m68runcmd -h:120 M68.68K -h:4 TRW.68K >>$outputfile
  fi

  rm $arg.s 2>/dev/null
  rm $arg.c 2>/dev/null
  rm $argu.68K 2>/dev/null
done

echo building ba
echo building ba >>$outputfile
cp ../c_tests/ba.c . >/dev/null
mt.sh ba $optflags $gccpath >>$outputfile
echo running ba
echo running ba >>$outputfile
$_m68runcmd BA.68K TP.BAS >>$outputfile
rm ba.s 2>/dev/null
rm ba.c 2>/dev/null
rm BA.68K 2>/dev/null

echo building an
echo building an >>$outputfile
cp ../c_tests/an.c . >/dev/null
mt.sh an $optflags $gccpath >>$outputfile
echo running an
echo running an >>$outputfile
$_m68runcmd AN.68K david lee >>$outputfile
rm an.s 2>/dev/null
rm an.c 2>/dev/null
rm AN.68K 2>/dev/null

echo building ff
echo building ff >>$outputfile
cp ../c_tests/ff.c . >/dev/null
cp ../c_tests/realpath.c . >/dev/null
cp ../c_tests/fnmatch.c . >/dev/null
mt.sh ff $optflags $gccpath >>$outputfile
echo running ff
echo running ff >>$outputfile
$_m68runcmd FF.68K -i "M*.68K" >>$outputfile
rm ff.s 2>/dev/null
rm ff.c 2>/dev/null
rm realpath.s 2>/dev/null
rm realpath.c 2>/dev/null
rm fnmatch.s 2>/dev/null
rm fnmatch.c 2>/dev/null
rm FF.68K 2>/dev/null

date_time=$(date)
echo "$date_time" >>$outputfile

diff --ignore-all-space baseline_$outputfile $outputfile

