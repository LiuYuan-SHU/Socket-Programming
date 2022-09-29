#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void error_handling(char* message);
void service(int serv_fd);

int main(int argc, char *argv[])
{
    // 程序参数检查
    if (argc != 3)
    {
        printf("Using:./client ip port\nExample:./client 127.0.0.1 5005\n\n");
        return -1;
    }

    // 创建一个Socket
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        return -1;
    }

    // 向服务器发起连接请求
    struct hostent *h;
    if ((h = gethostbyname(argv[1])) == 0) // 指定服务端的ip地址
    {
        printf("gethostbyname failed.\n");
        close(sockfd);
        return -1;
    }
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2])); // 指定服务端的通信端口
    memcpy(&servaddr.sin_addr, h->h_addr, h->h_length);
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) // 向服务端发起连接请求
    {
        perror("connect");
        close(sockfd);
        return -1;
    }

	service(sockfd);

    // 关闭socket
    close(sockfd);

	return 0;
}

void error_handling(char* message)
{
	fputs(message, stdout);
	fputc('\n', stdout);
	exit(1);
}

void service(int serv_fd)
{
	pid_t pid = fork();	

	if (pid == -1) { error_handling("fork() error"); }
	else if (pid == 0)
	{
		char buffer[1024];

		while (1)
		{
			int iret;
			
			// send message
			memset(buffer, 0, sizeof(buffer));
			printf(": ");
			fgets(buffer, sizeof(buffer) - 1, stdin);
			if ((iret = send(serv_fd, buffer, strlen(buffer), 0)) <= 0)
			{
				printf("iret = %d\n", iret);
			}
		}
	}
	else
	{
		char buffer[1024];
	
		// 与服务端通信，发送一个报文后等待回复，然后再发下一个报文
		while (1)
		{
			int iret;
			
			memset(buffer, 0, sizeof(buffer));
			if ((iret = recv(serv_fd, buffer, sizeof(buffer), 0)) <= 0)
			{
				printf("iret = %d\n", iret);
				break;
			}
			printf("server: %s", buffer);
		}
	}
}
