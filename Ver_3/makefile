all : server client

server : ./server.c
	gcc -Wall -g -o server server.c

client : ./client.cpp
	g++ -Wall -g -o client client.cpp

clean : 
	-rm -rf server ./server.dSYM
	-rm -rf client ./client.dSYM
