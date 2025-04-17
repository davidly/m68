#!/bin/bash
#set -x

for arg in hidave tprintf tm tmuldiv ttt sieve e tstr targs tbits t tao \
             tcmp ttypes tarray trw an terrno mm_old ttime fileops tpi \
             t_setjmp ba td tf tap tphi mm ts glob nantst pis
do
    echo $arg
    m.sh $arg
done

echo build m68.elf
mm68.sh

for arg in tbcd tshift taddsubm
do
    echo $arg
    ma.sh $arg
done



