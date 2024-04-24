all:
	mkdir -p build
	gcc -o build/server server.c -lpthread

run:
	./build/server
