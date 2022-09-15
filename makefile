all : server client

server : server.cpp
	g++  -Wall -g -o server server.cpp

client : client.cpp
	g++ -Wall -g -o client client.cpp
	
clean:
	-rm -rf server client ./client.dSYM ./server.dSYM
