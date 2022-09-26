all : server client

server : ./server.c
	gcc -Wall -o server server.c

client : ./client.c
	gcc -Wall -o client client.c

clean : 
	-rm -rf server ./server.dSYM
	-rm -rf client ./client.dSYM
	-rm ./logs/*
