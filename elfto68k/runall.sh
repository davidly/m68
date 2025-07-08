#!/bin/bash

if [ "$1" = "nested" ]; then
    _m68runcmd="../m68 -h:80 ../c_tests/m68.elf"
fi

if [ "$1" = "nested68k" ]; then
    _m68runcmd="../m68 -h:80 m68.68k"
fi

if [ "$1" = "armos" ]; then
    _m68runcmd="../../ArmOS/armos -h:80 ../../ArmOS/bin/m68"
fi

if [ "$1" = "rvos" ]; then
    _m68runcmd="../../rvos/rvos -h:80 ../../rvos/bin/m68.elf"
fi

if [ "$_m68runcmd" = "" ]; then
    _m68runcmd="../m68"
fi

outputfile="test_elfto68k.txt"
date_time=$(date)
echo "$date_time" >$outputfile

echo building elfto68k
echo building elfto68k >>$outputfile
m.sh >>$outputfile

echo building m68
echo building m68 >>$outputfile
mm68.sh >>$outputfile

for arg in hidave tprintf tm tmuldiv ttt sieve e tstr targs tbits t tao \
           tcmp ttypes tarray trw trw2 mm_old fileops tpi \
           t_setjmp td tf tap tphi mm ts glob nantst pis tfo \
           fopentst lenum tex trename

do
  echo building $arg
  echo building $arg >>$outputfile
  cp ../c_tests/$arg.c . >/dev/null
  mt.sh $arg >>$outputfile
  echo running $arg
  echo running $arg >>$outputfile
  argu=$(tr '[a-z]' '[A-Z]' <<< $arg)
  $_m68runcmd $argu.68K >>$outputfile
done

echo running m68.68k
echo running m68.68k >>$outputfile
$_m68runcmd M68.68K -h:4 TRW.68K >>$outputfile

echo building ba
echo building ba >>$outputfile
cp ../c_tests/ba.c . >/dev/null
mt.sh ba >>$outputfile
echo running ba
echo running ba >>$outputfile
$_m68runcmd BA.68K TP.BAS >>$outputfile

echo building an
echo building an >>$outputfile
cp ../c_tests/an.c . >/dev/null
mt.sh an >>$outputfile
echo running an
echo running an >>$outputfile
$_m68runcmd AN.68K david lee >>$outputfile

date_time=$(date)
echo "$date_time" >>$outputfile

diff --ignore-all-space baseline_$outputfile $outputfile

