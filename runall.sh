#!/bin/bash

_m68cmd="m68 -h:60"
if [ "$1" != "" ]; then
    _m68cmd="m68 -h:80 c_tests/m68.elf -h:60"
fi    

outputfile="test_m68.txt"
date_time=$(date)
echo "$date_time" >$outputfile

for arg in sieve e ttt tm ts tpi tmuldiv tstr mm tprintf tshift
do
    echo test $arg.hex
    echo test $arg.hex >>$outputfile
    $_m68cmd hexapps/$arg.hex >>$outputfile
done

for arg in hidave tprintf tm tmuldiv ttt sieve e tstr targs tbits t tao \
             tcmp ttypes tarray trw terrno mm_old ttime fileops tpi \
             t_setjmp td tf tap tphi mm ts glob nantst pis \
             tbcd tshift taddsubm;
do
    echo $arg
    echo test $arg >>$outputfile
    $_m68cmd c_tests/$arg.elf >>$outputfile
done

echo test an
echo test an -t david lee >>$outputfile
$_m68cmd c_tests/an.elf -t david lee >>$outputfile

echo test ba
echo test ba tp.bas >>$outputfile
$_m68cmd c_tests/ba.elf c_tests/tp.bas >>$outputfile

echo test m68.elf ttt 1
echo test m68.elf ttt 1 >>$outputfile
$_m68cmd c_tests/m68.elf c_tests/ttt.elf 1 >>$outputfile

for codegen in 6 8 a d 3 i I m o r x;
do
    $_m68cmd c_tests/ba.elf -a:$codegen -x c_tests/tp.bas >>$outputfile
done

date_time=$(date)
echo "$date_time" >>$outputfile
unix2dos $outputfile
#
diff --ignore-all-space baseline_$outputfile $outputfile
