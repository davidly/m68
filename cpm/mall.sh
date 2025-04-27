#!/bin/bash
#set -x

for arg in e sieve ttt tm fileops tpi
do
    echo building $arg
    m.sh $arg
done

for arg in ttt68u
do
    echo building $arg
    ma.sh $arg
done


