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

$_m68runcmd -h:1 PASCAL.68K $str.PAS
$_m68runcmd -h:1 CODE.68K $str.I
$_m68runcmd -h:1 ULINKER.68K -l $str.O $str.OBJ FTNLIB.OBJ PASLIB.OBJ
rm $str.OBJ 2>/dev/null
$_m68runcmd  -h:1 LO68.68K -s -o $str.68K -u__optoff -t10100 S.O $str.O CLIB

rm $str.O 2>/dev/null



