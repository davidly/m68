#!/bin/bash
#set -x

# build an assembly app that doesn't link with the C runtime.

if [ "$1" = "" ]; then
  echo no file specified
  exit 1
fi

if [ "$_m68runcmd" = "" ]; then
  _m68runcmd=../m68
fi

str=$(tr '[a-z]' '[A-Z]' <<< $1)

rm $str.68K 2>/dev/null
$_m68runcmd AS68.68K -l -u $str.S
            
$_m68runcmd LO68.68K -x -r -o $str.68K $str.O
rm $str.O



