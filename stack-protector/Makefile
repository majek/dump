.PHONY: all clean

MY_CFLAGS = $(CFLAGS) -Os -Wno-unused-parameter -Wall -Wextra -g -ggdb -fno-omit-frame-pointer

all: unprotected protected unfortified fortified

unprotected: protector.c Makefile
	$(CC) $(MY_CFLAGS) protector.c -o $@ -fno-stack-protector -U_FORTIFY_SOURCE

protected: protector.c Makefile
	$(CC) $(MY_CFLAGS) protector.c -o $@ -fstack-protector -U_FORTIFY_SOURCE

unfortified: fortify.c Makefile
	$(CC) $(MY_CFLAGS) fortify.c -o $@ -fno-stack-protector -U_FORTIFY_SOURCE

fortified: fortify.c Makefile
	$(CC) $(MY_CFLAGS) fortify.c -o $@ -fno-stack-protector -D_FORTIFY_SOURCE=2

clean:
	rm -rf protected unprotected fortified unfortified