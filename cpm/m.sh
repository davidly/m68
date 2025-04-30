#!/bin/bash
#set -x

if [ "$1" = "" ]; then
  echo no file specified
  exit 1
fi

if [ "$_m68runcmd" = "" ]; then
  _m68runcmd=../m68
fi

str=$(tr '[a-z]' '[A-Z]' <<< $1)

rm $str.68K 2>/dev/null
$_m68runcmd cp68.68k $str.C $str.I
$_m68runcmd c068.68k $str.I $str.1 $str.2 $str.3 -f
rm $str.I 2>/dev/null
$_m68runcmd c168.68k $str.1 $str.2 $str.S
rm $str.1 2>/dev/null
rm $str.2 2>/dev/null

$_m68runcmd as68.68k -l -u $str.S
rm $str.S 2>/dev/null

$_m68runcmd lo68.68k -r -u_nofloat -o $str.68K S.O $str.O clib
rm $str.O 2>/dev/null
