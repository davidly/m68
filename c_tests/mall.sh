#!/bin/bash
#set -x

if [ "$1" == "" ]; then
    optflags="2"
else
    optflags="$1"
fi

if [ "$2" == "" ]; then
    gccver="8.2.0"
#    gccver="13.2.0"
else
    gccver=$2
fi

for arg in hidave tprintf tm tmuldiv ttt sieve e tstr targs tbits t tao \
           tcmp ttypes tarray trw trw2 an terrno mm_old ttime fileops tpi \
           t_setjmp ba td tf tap tphi mm ts glob nantst pis tfo sleeptm ff \
           nqueens nq1d tdir fopentst lenum tgets tex trename tmmap tenv \
           na termiosf wumpus taux
do
    echo $arg
    m.sh $arg $optflags $gccver
done

echo build m68.elf
mm68.sh $optflags $gccver

for arg in tbcd tshift taddsubm tea ttt68 ttt68u tchk tsh
do
    echo $arg
    ma.sh $arg $optflags $gccver
done



