#include <cstdio>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    // 对程序调用参数进行判断
    if (argc != 2)
    {
        printf("Using:./server port\nExample:./server 5005\n\n");
        return -1;
    }

    // 第一步，创建服务端Socket
    int listenfd;
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        return -1;
    }

    // 第二步，把服务端用于通信的端口和地址绑定到Socket
    struct sockaddr_in servaddr; // 服务端地址信息数据结构
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;				  // 协议族，在Socket编程中只能是AF_INET
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 任意IP地址
    // servaddr.sin_addr.s_addr = inet_addr("192.168.190.134"); // 指定IP地址
    servaddr.sin_port = htons(atoi(argv[1])); // 指定通信端口，使用网络字节序
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
    {
        perror("bind");
        close(listenfd);
        return -1;
    }

    // 第三步，把Socket设置为监听模式
    if (listen(listenfd, 5) != 0)
    {
        perror("listen");
        close(listenfd);
        return -1;
    }

    // 第四步：接受客户端的连接
    int clientfd;							  // 客户端Socket
    int socklen = sizeof(struct sockaddr_in); // struct sockaddr_in的大小
    struct sockaddr_in clientaddr;			  // 客户端的地址信息
    clientfd = accept(listenfd, (struct sockaddr *)&clientaddr, (socklen_t *)&socklen);
	const char* client_ip = inet_ntoa(clientaddr.sin_addr);
    printf("Client(%s) connected\n", client_ip);

    // 第五步：与客户端通信，接收客户端发过来的报文后，回复OK
    char buffer[1024];
    while (1)
    {
        int iret;
        memset(buffer, 0, sizeof(buffer));
        if ((iret = recv(clientfd, buffer, sizeof(buffer), 0)) <= 0) // 接收客户端的请求报文
        {
            printf("iret=%d\n", iret);
            break;
        }
		printf("%s: %s\n", client_ip, buffer);

		memset(buffer, 0, sizeof(buffer));
		printf(": ");
		gets(buffer);
        if ((iret = send(clientfd, buffer, strlen(buffer), 0)) <= 0) // 向客户端发送响应结果
        {
            perror("send");
            break;
        }
		printf("========== MESSAGE SENT ==========\n");
    }

    // 第六步：关闭socket
    close(listenfd);
    close(clientfd);
}

