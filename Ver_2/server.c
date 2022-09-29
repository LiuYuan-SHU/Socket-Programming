#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024

void error_handling(char* message);

void service(int clnt_fd);

int main(int argc, char* argv[])
{
	if (argc != 2) 
	{
		fprintf(stdout, "Usage : %s <port>", argv[0]);
	}

	int listen_fd;
	if ((listen_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
	{ error_handling("socket() error"); }

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));

	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_sz = sizeof(clnt_addr);

	if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
	{ error_handling("bind() error"); }

	if (listen(listen_fd, 5) == -1)
	{ error_handling("listen() error"); }

	pid_t pid;
	while (1)
	{
		int clnt_fd;
		if ((clnt_fd = accept(listen_fd, (struct sockaddr *restrict)&clnt_addr, &clnt_addr_sz)) == -1)
		{ error_handling("accept() error"); }

		fprintf(stdout, "========== Connection Acctped ==========\n");
		fprintf(stdout, "IP: %s \n", inet_ntoa(clnt_addr.sin_addr));
		fprintf(stdout, "port: %d \n", ntohs(clnt_addr.sin_port));
		fprintf(stdout, "========================================\n");

		pid = fork();

		if (pid == -1) { error_handling("fork() error"); }
		else if (pid == 0) 
		{
			close(listen_fd);
			service(clnt_fd);
			exit(0);
		}
		else
		{
			close(clnt_fd);
		}
	}

	close(listen_fd);

	return 0;
}

void error_handling(char* message)
{
	fputs(message, stdout);
	fputc('\n', stdout);
	exit(1);
}

void service(int clnt_fd)
{
	char* message = "Hello world!\n";
	write(clnt_fd, message, strlen(message));
}
