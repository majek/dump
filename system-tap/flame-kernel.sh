#!/bin/bash

sudo stap -s 32 --all-modules -v \
    -D MAXBACKTRACE=100 \
    -D MAXSTRINGLEN=4096 \
    -D MAXMAPENTRIES=10240  \
    -D DMAXACTION_INTERRUPTIBLE=500 \
    -D MAXACTION=100000 \
    -D STP_OVERLOAD_THRESHOLD=5000000000 \
    flame-kernel.stp \
    > out.stap-stacks
