#!/bin/bash
#set -x

if [ "$1" = "nested" ]; then
    _m68runcmd="../m68 -h:100 ../c_tests/m68.elf"
fi

if [ "$1" = "armos" ]; then
    _m68runcmd="../../ArmOS/armos -h:100 ../../ArmOS/bin/m68"
fi

if [ "$1" = "rvos" ]; then
    _m68runcmd="../../rvos/rvos -h:100 ../../rvos/bin/m68.elf"
fi

if [ "$_m68runcmd" = "" ]; then
    _m68runcmd="../m68"
fi

for arg in E SIEVE TTT MM TPHI TAP NQUEENS NQ1D
do
    echo running $arg
    $_m68runcmd $arg.68K
done
