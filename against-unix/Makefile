

run:
	gcc -O3 fatal.c -o fatal
	gcc -O3 client.c -o client
	(./fatal 256 1021 &);
	@sleep 10
	./client
	kill `pidof fatal`

