
all:
	gcc -O2 -g -fno-omit-frame-pointer  -Wall -Wextra ohamt_mem.c ohamt.c ohamt_sort.c -lcrypto -oohamt_sort


in2:
	cat /dev/urandom |tr -dc "a-z\n0-9"|tr -c "a-z" "\n"|head -n 100000 > in2
