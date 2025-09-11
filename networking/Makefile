CC = gcc
CFLAGS = -Wall -g

all: client server

client: client.c sham.h
	$(CC) $(CFLAGS) client.c -o client -lcrypto

server: server.c sham.h
	$(CC) $(CFLAGS) server.c -o server -lcrypto

clean:
	rm -f client server *.o server_log.txt client_log.txt
