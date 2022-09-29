# Computer-Network

## Ver 1

```shell
# run
make
./server 5005
./client localhost 5005
```

## Ver 2

```shell
# run
make
./server 5005
# to test multiple connections, run this script in many terminals:
./client localhost 5005
```

## Ver 3

multiple processes

## Ver 4

big file transport, breakpoint check

```mermaid
flowchart TD;
subgraph client
	create[create connection with server] --> 
	cservice[function <b>service</b>] --> send & receive
	send --> file[send file]
end

subgraph server
	main --> 
	listen[listen connection] --fork--> 
	accept[accept connection] & listen 
	accept --> sservice[function service] --fork--> my_send & my_recv
	my_recv --> save_file
end
```
