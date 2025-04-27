#!/bin/bash
#set -x

export M68Nested

for arg in e sieve ttt
do
    echo building $arg
    m.sh $arg
done
