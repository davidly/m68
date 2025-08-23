#!/bin/bash
#set -x

if [ "$1" == "" ]; then
    optflags="2"
else
    optflags="$1"
fi

if [ "$2" == "" ]; then
    gccpath="../gcc-8.2.0-linux"
else
    gccpath=$2
fi

for arg in hidave tprintf tm tmuldiv ttt sieve e tstr targs tbits t tao \
             tcmp ttypes tarray trw trw2 an terrno mm_old ttime fileops tpi \
             t_setjmp ba td tf tap tphi mm ts glob nantst pis tfo sleeptm ff \
             nqueens nq1d tdir fopentst lenum tgets tex trename
do
    echo $arg
    m.sh $arg $optflags $gccpath
done

echo build m68.elf
mm68.sh $optflags $gccpath

for arg in tbcd tshift taddsubm tea ttt68 ttt68u tchk
do
    echo $arg
    ma.sh $arg $optflags $gccpath
done



