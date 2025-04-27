#!/bin/bash

if [ "$M68Nested" = "" ]; then
  _runcmd="../m68"
else
  _runcmd="../m68 -h:60 ../c_tests/m68.elf"
fi

for arg in E SIEVE TTT
do
  echo running $arg
  $_runcmd $arg.68K
done
