#!/bin/bash
#set -x

str=$(tr '[a-z]' '[A-Z]' <<< $1)

rm $str.68K 2>/dev/null
../m68 as68.68k -l -u $str.S

../m68 lo68.68k -r -u_nofloat -o $str.68K S.O $str.O CLIB
rm $str.O



