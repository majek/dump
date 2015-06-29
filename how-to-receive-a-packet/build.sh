#!/bin/sh
set +e


clang -O3 -Wall -Wextra -Wno-unused-parameter -Wpointer-arith -Wl,-z,now -Wl,-z,relro -pie -fPIC -D_FORTIFY_SOURCE=2\
    -ggdb -g -pthread \
    -o udpserver \
    udpserver.c net.c

clang -O3 -Wall -Wextra -Wno-unused-parameter -Wpointer-arith -Wl,-z,now -Wl,-z,relro -pie -fPIC -D_FORTIFY_SOURCE=2\
    -ggdb -g -pthread -lm \
    -o udpclient \
    udpclient.c net.c
