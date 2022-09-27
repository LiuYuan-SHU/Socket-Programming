all : client server

client : ./client.c 
	gcc -DSLEEP -o client client.c
	gcc -o client_no_sleep client.c

server : ./server.c
	gcc -o server server.c

clean:
	-rm client client_no_sleep
	-rm server
	-rm ./logs/*
	-rm *_server
