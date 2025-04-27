#!/bin/bash
#set -x

if [ "$1" = "" ]; then
  echo no file specified
  exit 1
fi

if [ "$M68Nested" = "" ]; then
  _runcmd="../m68"
else
  _runcmd="../m68 -h:60 ../c_tests/m68.elf"
fi

str=$(tr '[a-z]' '[A-Z]' <<< $1)

rm $str.68K 2>/dev/null
$_runcmd MT68.68K $str.PAS

$_runcmd link68.68k $str,PASLIB.L68
rm $str.O 2>/dev/null
