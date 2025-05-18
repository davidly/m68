#!/bin/bash

if [ "$1" = "nested" ]; then
    _m68runcmd="../m68 -h:60 ../c_tests/m68.elf"
fi

if [ "$1" = "armos" ]; then
    _m68runcmd="../../ArmOS/armos -h:60 ../../ArmOS/bin/m68"
fi

if [ "$1" = "rvos" ]; then
    _m68runcmd="../../rvos/rvos -h:60 ../../rvos/bin/m68.elf"
fi

if [ "$_m68runcmd" = "" ]; then
    _m68runcmd="../m68"
fi

for arg in E SIEVE TTT TM FILEOPS TPI TTT68U T_SETJMP TMULDIV TCHK TMOVEP MM E68 SIEVE68
do
  echo running $arg
  $_m68runcmd -h:1 $arg.68K
done

# compile and run a basic app
$_m68runcmd -h:1 ../c_tests/ba -x -a:c ../c_tests/tp.bas
cp ../c_tests/tp.s TP.S
unix2dos TP.S
ma.sh TP
$_m68runcmd -h:1 TP.68K
