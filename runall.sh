#!/bin/bash
#set -x

# separate tests are in elfto68k and aztec68k/tests runall scripts

_runcmd="m68"
_m68runcmd="../m68"

if [ "$1" = "nested" ]; then
    _runcmd="m68 -h:180 c_tests/m68.elf"
    _m68runcmd="../m68 -h:180 ../c_tests/m68.elf"
elif [ "$1" = "armos" ]; then
    _runcmd="../ArmOS/armos -h:180 ../ArmOS/bin/m68"
    _m68runcmd="../../ArmOS/armos -h:180 ../../ArmOS/bin/m68"
elif [ "$1" = "sparcos" ]; then
    _runcmd="../sparcos/sparcos -h:180 ../sparcos/bin/m68"
    _m68runcmd="../../sparcos/sparcos -h:180 ../../sparcos/bin/m68 "
elif [ "$1" = "rvos" ]; then
    _runcmd="../rvos/rvos -h:180 ../rvos/bin/m68 "
    _m68runcmd="../../rvos/rvos -h:180 ../../rvos/bin/m68 "
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
           tcmp ttypes tarray trw trw2 terrno mm_old ttime fileops tpi \
           t_setjmp td tf tap tphi mm ts glob nantst pis tfo sleeptm \
           nqueens nq1d tdir fopentst lenum tex trename tmmap termiosf \
           tbcd tshift taddsubm tea ttt68 ttt68u tchk tsh;
do
    echo $arg
    echo test $arg >>$outputfile
    $_runcmd c_tests/$arg.elf >>$outputfile
done

for arg in cpm mtpascal cb68 cpmcv11 cpmcv12 svspas svsfor forth83
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
$_runcmd c_tests/ba.elf -q c_tests/tp.bas >>$outputfile

echo test m68.elf ttt.elf 1
echo test m68.elf ttt.elf 1 >>$outputfile
$_runcmd -h:120 c_tests/m68.elf c_tests/ttt.elf 1 >>$outputfile

echo test m68.elf ttt.68k 1
echo test m68.elf ttt.68k 1 >>$outputfile
$_runcmd -h:120 c_tests/m68.elf cpm/TTT.68K 1 >>$outputfile

echo test com cp/m 2.2 emulator
echo test com cp/m 2.2 emulator >>$outputfile
pushd com 1>/dev/null
$_m68runcmd -h:1 COM.68K MBASIC.COM HELLO.BAS >>../$outputfile
popd 1>/dev/null

for codegen in 6 8 a d 3 i I m o r x;
do
    $_runcmd c_tests/ba.elf -a:$codegen -q -x c_tests/tp.bas >>$outputfile
done

# run app with redirected input
echo running tgets with redirected stdin
echo running tgets with redirected stdin >>$outputfile
$_runcmd c_tests/tgets <tgets.txt >>$outputfile

echo test ff . ff.c
echo test ff . ff.c >>$outputfile
$_runcmd c_tests/ff . ff.c >>$outputfile

echo test targs
echo test targs a bb ccc dddd >>$outputfile
$_runcmd c_tests/targs.elf a bb ccc dddd >>$outputfile

date_time=$(date)
echo "$date_time" >>$outputfile
unix2dos -f $outputfile
#
diff --ignore-all-space baseline_$outputfile $outputfile
