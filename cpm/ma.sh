#!/bin/bash
#set -x

str=$(tr '[a-z]' '[A-Z]' <<< $1)

../m68 as68.68k -l -u -s 0: $str.S
../m68 lo68.68k -r -u_nofloat -o $str.68K 0:S.O $str.O 0:CLIB



