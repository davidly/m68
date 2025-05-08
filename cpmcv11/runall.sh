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

for arg in E SIEVE TTT TM TTT68U T_SETJMP TCHK TMOVEP MM
do
  echo running $arg
  # the C runtime zeroes bss, so use a small bss
  $_m68runcmd -h:1 $arg.68K
done
