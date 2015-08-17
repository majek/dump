

run:
	gcc -O3 fatal.c -o fatal
	gcc -O3 client.c -o client
	(ulimit -n 1024; ./fatal 512 1021 &);
	@sleep 4
	./client
	kill `pgrep fatal`

