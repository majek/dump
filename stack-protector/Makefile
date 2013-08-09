.PHONY: all clean

MY_CFLAGS = $(CFLAGS) -Os -Wno-unused-parameter -Wall -Wextra -g -ggdb -m32

all: unprotected protected unfortified fortified omitted notomitted

unprotected: protector.c Makefile
	$(CC) $(MY_CFLAGS) protector.c -o $@ -fno-stack-protector -U_FORTIFY_SOURCE -fno-omit-frame-pointer

protected: protector.c Makefile
	$(CC) $(MY_CFLAGS) protector.c -o $@ -fstack-protector -U_FORTIFY_SOURCE -fno-omit-frame-pointer

unfortified: fortify.c Makefile
	$(CC) $(MY_CFLAGS) fortify.c -o $@ -fno-stack-protector -U_FORTIFY_SOURCE -fno-omit-frame-pointer

fortified: fortify.c Makefile
	$(CC) $(MY_CFLAGS) fortify.c -o $@ -fno-stack-protector -D_FORTIFY_SOURCE=2 -fno-omit-frame-pointer

omitted: framepointer.c Makefile
	$(CC) $(MY_CFLAGS) framepointer.c -o $@ -fomit-frame-pointer
notomitted: framepointer.c Makefile
	$(CC) $(MY_CFLAGS) framepointer.c -o $@ -fno-omit-frame-pointer

clean:
	rm -rf protected unprotected fortified unfortified
