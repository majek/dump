all: server client



libmill/.libs/libmill.la:
	-$(MAKE) -C libmill distclean
	if [ ! -e libmill/configure ]; then (cd libmill && ./autogen.sh); fi
	(cd libmill && \
		./configure CFLAGS="-fstack-protector-all")
	$(MAKE) -C libmill libmill.la

#CFLAGS=-fPIC
#		-Wl,-z,now -Wl,-z,relro -pie\
#		-fPIE  \
#		-fPIC \

server: server.c libmill/.libs/libmill.la Makefile misc.c
	gcc -ggdb -g -O2 \
		-Wall -Wextra  -Wno-unused-parameter -Wpointer-arith -Werror \
		-D_FORTIFY_SOURCE=2 \
		misc.c \
		server.c \
		libmill/.libs/libmill.a \
		-I. \
		-o server

client: client.c libmill/.libs/libmill.la Makefile misc.c
	gcc -ggdb -g -O2 \
		-Wall -Wextra  -Wno-unused-parameter -Wpointer-arith -Werror \
		-D_FORTIFY_SOURCE=2 \
		misc.c \
		client.c \
		libmill/.libs/libmill.a \
		-I. \
		-pthread \
		-o client

