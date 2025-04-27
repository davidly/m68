#!/bin/bash
#set -x

for arg in e sieve ttt
do
    echo building $arg
    m.sh $arg
done
