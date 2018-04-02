all: server client
server: server.c
	gcc -Wall -g -o server server.c -lrt -lpthread
client: client.c
	gcc -Wall -g -o client client.c -lrt
