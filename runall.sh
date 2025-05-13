#!/bin/bash

_runcmd="m68"
_m68runcmd="../m68"

if [ "$1" != "" ]; then
    _runcmd="m68 -h:80 c_tests/m68.elf"
    _m68runcmd="../m68 -h:80 ../c_tests/m68.elf"
fi

export _m68runcmd

outputfile="test_m68.txt"
date_time=$(date)
echo "$date_time" >$outputfile

for arg in sieve e ttt tm ts tpi tmuldiv tstr mm tprintf tshift
do
    echo test $arg.hex
    echo test $arg.hex >>$outputfile
    $_runcmd hexapps/$arg.hex >>$outputfile
done

for arg in hidave tprintf tm tmuldiv ttt sieve e tstr targs tbits t tao \
           tcmp ttypes tarray trw terrno mm_old ttime fileops tpi \
           t_setjmp td tf tap tphi mm ts glob nantst pis tfo sleeptm \
           tbcd tshift taddsubm tea ttt68 ttt68u;
do
    echo $arg
    echo test $arg >>$outputfile
    $_runcmd c_tests/$arg.elf >>$outputfile
done

for arg in cpm mtpascal cb68 cpmcv11 cpmcv12
do
    echo compiler test $arg
    echo compiler test $arg >>$outputfile
    pushd $arg 1>/dev/null
    mall.sh >>../$outputfile
    runall.sh >>../$outputfile
    popd 1>/dev/null
done

echo test an
echo test an -t david lee >>$outputfile
$_runcmd c_tests/an.elf -t david lee >>$outputfile

echo test ba
echo test ba tp.bas >>$outputfile
$_runcmd c_tests/ba.elf c_tests/tp.bas >>$outputfile

echo test m68.elf ttt.elf 1
echo test m68.elf ttt.elf 1 >>$outputfile
$_runcmd -h:60 c_tests/m68.elf c_tests/ttt.elf 1 >>$outputfile

echo test m68.elf ttt.68k 1
echo test m68.elf ttt.68k 1 >>$outputfile
$_runcmd -h:60 c_tests/m68.elf cpm/TTT.68K 1 >>$outputfile

for codegen in 6 8 a d 3 i I m o r x;
do
    $_runcmd c_tests/ba.elf -a:$codegen -x c_tests/tp.bas >>$outputfile
done

date_time=$(date)
echo "$date_time" >>$outputfile
unix2dos -f $outputfile
#
diff --ignore-all-space baseline_$outputfile $outputfile
