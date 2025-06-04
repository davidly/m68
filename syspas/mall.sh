#!/bin/bash
#set -x

if [ "$1" = "nested" ]; then
        _m68runcmd="../m68 -h:60 ../c_tests/m68.elf"
fi

if [ "$1" = "armos" ]; then
        _m68runcmd="../../ArmOS/armos -h:60 ../../ArmOS/bin/m68"
fi

if [ "$1" = "rvos" ]; then
        _m68runcmd="../../rvos/rvos -h:60 ../../rvos/bin/m68"
fi

if [ "$_m68runcmd" = "" ]; then
  _runcmd="../m68"
fi

export _m68runcmd

for arg in e sieve ttt mm
do
    echo building $arg
    m.sh $arg
done
