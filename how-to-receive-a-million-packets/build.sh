#!/bin/sh
set +e

clang -O3 -Wall -Wextra -Wno-unused-parameter \
    -ggdb -g -pthread \
    -o udpreceiver1 udpreceiver1.c \
    net.c

clang -O3 -Wall -Wextra -Wno-unused-parameter \
    -ggdb -g -pthread \
    -o udpsender udpsender.c \
    net.c
