#!/bin/bash
#set -x

# build a C app that uses Motorola's floating point format

if [ "$1" = "" ]; then
  echo no file specified
  exit 1
fi

if [ "$_m68runcmd" = "" ]; then
  _m68runcmd=../m68
fi

str=$(tr '[a-z]' '[A-Z]' <<< $1)

rm $str.68K 2>/dev/null
$_m68runcmd CP68.68K $str.C $str.I
$_m68runcmd C068.68K $str.I $str.1 $str.2 $str.3 -f
rm $str.I 2>/dev/null
$_m68runcmd C168.68K $str.1 $str.2 $str.S
rm $str.1 2>/dev/null
rm $str.2 2>/dev/null

$_m68runcmd AS68.68K -l -u $str.S
rm $str.S 2>/dev/null

# libf12.a is from v1.2 of the compiler and it works well but is missing some functions in v1.3's libf.a
$_m68runcmd LO68.68K -r -u__optoff  -o $str.68K S.O $str.O CLIB LIBF12.A LIBF.A
rm $str.O 2>/dev/null
