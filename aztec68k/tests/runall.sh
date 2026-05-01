#!/bin/bash
#set -x

_runcmd=../../m68

outputfile="test_aztec.txt"
date_time=$(date)
echo "$date_time" >$outputfile

# build libraries
mlib.sh >>$outputfile

# build symto68k utility
msymto68k.sh >>$outputfile

# tests without floating point build with m.sh
for arg in SIEVE E TTT TM TARGS HI TPI MM NQ1D PRIMES NQUEENS T_SETJMP FOPENTST TRW TRW2 FILEOPS TRENAME
do
    echo test $arg
    echo test $arg >>$outputfile
    m.sh $arg >>$outputfile
    $_runcmd $arg.68K >>$outputfile
    rm $arg.R
    rm $arg.SYM
    rm $arg.68K
done

# tests with floating point build with mf.sh
for arg in TPHI TAP TMULDIV PIS MANDLE TS TF
do
    echo test $arg
    echo test $arg >>$outputfile
    mf.sh $arg >>$outputfile
    $_runcmd $arg.68K >>$outputfile
    rm $arg.R
    rm $arg.SYM
    rm $arg.68K
done

date_time=$(date)
echo "$date_time" >>$outputfile
diff --ignore-all-space baseline_$outputfile $outputfile
