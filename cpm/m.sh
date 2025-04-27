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
$_runcmd cp68.68k $str.C $str.I
$_runcmd c068.68k $str.I $str.1 $str.2 $str.3 -f
rm $str.I 2>/dev/null
$_runcmd c168.68k $str.1 $str.2 $str.S
rm $str.1 2>/dev/null
rm $str.2 2>/dev/null

$_runcmd as68.68k -l -u $str.S
rm $str.S 2>/dev/null

$_runcmd lo68.68k -r -u_nofloat -o $str.68K S.O $str.O clib
rm $str.O 2>/dev/null
