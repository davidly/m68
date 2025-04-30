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
$_m68runcmd as68.68k -l -u $str.S

$_m68runcmd lo68.68k -r -u_nofloat -o $str.68K S.O $str.O CLIB
rm $str.O



