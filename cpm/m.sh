#!/bin/bash
#set -x

str=$(tr '[a-z]' '[A-Z]' <<< $1)

../m68 cp68.68k -i 0: $str.C $str.I
../m68 c068.68k $str.I $str.1 $str.2 $str.3 -f
rm $str.I
../m68 c168.68k $str.1 $str.2 $str.S
rm $str.1
rm $str.2
../m68 as68.68k -l -u -s 0: $str.S
#rm  $str.S

../m68 lo68.68k -r -u_nofloat -o $str.68K 0:S.O $str.O 0:clib
