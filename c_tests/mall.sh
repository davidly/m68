#!/bin/bash
#set -x

for arg in hidave tprintf tm tmuldiv ttt sieve e tstr targs tbits t tao \
             tcmp ttypes tarray trw trw2 an terrno mm_old ttime fileops tpi \
             t_setjmp ba td tf tap tphi mm ts glob nantst pis tfo sleeptm ff \
             nqueens tdir fopentst lenum tgets tex trename
do
    echo $arg
    m.sh $arg
done

echo build m68.elf
mm68.sh

for arg in tbcd tshift taddsubm tea ttt68 ttt68u tchk
do
    echo $arg
    ma.sh $arg
done



