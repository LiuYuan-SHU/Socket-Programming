#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024

void error_handling(char* message);
void service(const char* ip, int clnt_fd);
void my_send(const char* ip, int clnt_fd);
void my_recv(const char* ip, int clnt_fd);

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
			service(inet_ntoa(clnt_addr.sin_addr), clnt_fd);
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

void service(const char* ip, int clnt_fd)
{
	pid_t pid = fork();

	if (pid == -1) { error_handling("fork() error"); }
	else if (pid == 0) { my_send(ip, clnt_fd); exit(0); }
	else { my_recv(ip, clnt_fd); }
}

void my_send(const char* ip, int clnt_fd)
{
	char buffer[1024];

	while (1)
	{
		int iret;
		memset(buffer, 0, sizeof(buffer));
		fprintf(stdout, "To %s : ", ip);
		fgets(buffer, sizeof(buffer) - 1, stdin);
		if (strcmp(buffer, "quit\n") == 0) { close(clnt_fd); break; } 
		if ((iret = send(clnt_fd, buffer, strlen(buffer), 0)) <= 0)
		{
			fprintf(stdout, "iret = %d \n", iret);
			break;
		}
	}
}

void my_recv(const char* ip, int clnt_fd)
{
	char buffer[1024];

	while (1)
	{
		int iret;
		memset(buffer, 0, sizeof(buffer));
		if ((iret = recv(clnt_fd, buffer, sizeof(buffer), 0)) <= 0 )
		{
			fprintf(stdout, "\n========================================\n");
			fprintf(stdout, "DISCONNECTED with iret = %d\n", iret);
			fprintf(stdout, "========================================\n");
			break;
		}
		if ( strcmp(buffer, "quit\n") == 0 ) { close(clnt_fd); break; }

		printf("\n%s: %s\n", ip, buffer);
		fflush(stdout);
	}
}
