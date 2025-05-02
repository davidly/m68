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
$_m68runcmd -h:1 CP.68K $str.C $str.I
$_m68runcmd -h:1 C0.68K $str.I $str.IC $str.ST
rm $str.I 2>/dev/null
$_m68runcmd -h:1 C1.68K $str.IC $str.S -l
rm $str.IC 2>/dev/null
rm $str.ST 2>/dev/null

$_m68runcmd -h:1 AS.68K -l -u $str.S
rm $str.S 2>/dev/null

$_m68runcmd -h:1 LO.68K -r -o $str.68K S.O $str.O clib
rm $str.O 2>/dev/null
