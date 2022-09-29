#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define MAX_SIZE 1024					// the max size of buffer to restore file name
#define BUF_SIZE 100					// the size of package data buffer
#define HED_SIZE 100					// the size of package header buffer
#define TOL_SIZE BUF_SIZE+HED_SIZE		// the total size of package
#define PLACE_HOLDER 1					// empty character fill

void error_handling(char* message);
void service(int serv_fd);
void send_file(int serv_fd);

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage:./client ip port\nExample:./client 127.0.0.1 5005\n\n");
        return -1;
    }

	// create socket
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        error_handling("socket");
        return -1;
    }

	// obtain the IP address based on the domain name
    struct hostent *h;
    if ((h = gethostbyname(argv[1])) == 0) // 指定服务端的ip地址
    {
        printf("gethostbyname failed.\n");
        close(sockfd);
        return -1;
    }

	// set the information of server
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2])); 
    memcpy(&servaddr.sin_addr, h->h_addr, h->h_length);

	// connect server
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) // 向服务端发起连接请求
    {
        error_handling("connect");
        close(sockfd);
        return -1;
    }

	// let service function handle the connection
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
	// get a new process
	pid_t pid = fork();	

	if (pid == -1) { error_handling("fork() error"); }
	// send message
	else if (pid == 0)
	{
		char buffer[1024];

		while (1)
		{
			int iret;
			
			memset(buffer, 0, sizeof(buffer));
			printf(": ");
			// get message from stdin
			fgets(buffer, sizeof(buffer) - 1, stdin);
			// send file
			if (strcmp("file\n", buffer) == 0)
			{
				send_file(serv_fd);
			}
			// send message
			else if ((iret = send(serv_fd, buffer, strlen(buffer), 0)) <= 0)
			{
				printf("iret = %d\n", iret);
			}
		}
	}
	// receive data from server
	else
	{
		char buffer[1024];
	
		while (1)
		{
			int iret;
			
			memset(buffer, 0, sizeof(buffer));
			if ((iret = recv(serv_fd, buffer, sizeof(buffer), 0)) <= 0)
			{
				printf("iret = %d\n", iret);
				break;
			}
			printf("server: %s\n", buffer);
			fflush(stdout);
		}
	}
}

void send_file(int serv_fd)
{
	int iret;
	// set the buffer for file name and package
	char file_name[MAX_SIZE], buffer[BUF_SIZE + HED_SIZE];
	FILE *file_ptr = NULL;
	FILE *log_ptr = NULL; 

	// initialize log_ptr
	// generate name of log file
	char log_name[MAX_SIZE];
	time_t cur_time = time(NULL);
	strcpy(log_name, "logs/log_");	
	strncat(log_name, ctime(&cur_time) + 11, 8);
	strcat(log_name, ".txt");
	// create log file
	log_ptr = fopen(log_name, "w");	

	// get file name
	printf("Input the file name: ");
	scanf("%s", file_name);
	file_ptr = fopen(file_name, "r");

	// if faile to open file
	if (!file_ptr)
	{
		printf("fopen() failed\n");
		printf("errno = %d, reason = %s\n", errno, strerror(errno));
		// system("pause");
	}
	else 
	{
		printf("file %s opened\n", file_name);
		// record package number
		size_t count = 0;
		// record how many bytes will be sent this time
		size_t sent_size = 0;

		// get the total size of file
		// file_stat to restore the info of file
		struct stat file_stat;
		stat(file_name, &file_stat);	
		// get the size of file
		const size_t file_size = file_stat.st_size;

		int flag = 1;
		while (flag)
		{
			// initialize buffer
			memset(buffer, PLACE_HOLDER, HED_SIZE);
			memset(buffer + HED_SIZE, 0, BUF_SIZE);

			// write header
			count++;
			int offset = sprintf(buffer, "FILE: %s - package %zu of total %zu:\n", file_name, count, (file_size % BUF_SIZE) == 0 ? (file_size / BUF_SIZE) : (file_size / BUF_SIZE + 1));	
			// replace the \0 character at the end of header
			buffer[offset] = PLACE_HOLDER;

			sent_size = fread(buffer + HED_SIZE, sizeof(char), BUF_SIZE, file_ptr);
			// if there is still package to send
			if (sent_size > 0)
			{
				// write log
				fprintf(log_ptr, "buffer:\n%s\n", buffer);
				fprintf(log_ptr, "===============================\n");

				// send package
				if ((iret = send(serv_fd, buffer, TOL_SIZE, 0)) <= 0)
				{
					printf("iret = %d\n", iret);
					flag = 0;
				}
				printf("FILE: %s - package %-5zu sent\n",file_name, count);
#ifdef SLEEP
				// sleep a while so that the server can receive the next package
				usleep(100000);
#endif
			}
			else
			{
				// end loop
				flag = 0;

				// send tail to server
				sprintf(buffer,  "FILE: %s - package %d\n", file_name, (((int)count) - 1) * -1);
				send(serv_fd, buffer, TOL_SIZE, 0);
			}
		}
	}
	fclose(file_ptr);
	fclose(log_ptr);
}
