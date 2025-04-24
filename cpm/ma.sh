#!/bin/bash
#set -x

str=$(tr '[a-z]' '[A-Z]' <<< $1)

../m68 as68.68k -l -u -s 0: %1.S
../m68 lo68.68k -r -u_nofloat -o %1.68K 0:S.O %1.O 0:clib



