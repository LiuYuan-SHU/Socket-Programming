#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define BUF_SIZE	1024
#define HED_SIZE	100
#define RECEIVED		1
#define NOT_RECEIVED	0

char receiving = 0;
time_t last_packge = 0;
char package_status[65536];
size_t package_total_num;

void error_handling(char* message);
void service(const char* ip, int clnt_fd);
void my_send(const char* ip, int clnt_fd);
void my_recv(const char* ip, int clnt_fd);
void save_file(const char* buffer, size_t size, int clnt_fd);
void call_resume(char* status, size_t length, size_t message_count, int clnt_fd);

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
		if ((clnt_fd = accept(listen_fd, (struct sockaddr *)&clnt_addr, &clnt_addr_sz)) == -1)
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
	memset(buffer, 0, sizeof(buffer));

	while (1)
	{
		// breakpoint resume
		if (receiving)	
		{
			call_resume(package_status, 1024, package_total_num, clnt_fd);
			fflush(stdout);
		}

		int iret;
		if ((iret = recv(clnt_fd, buffer, sizeof(buffer), 0)) <= 0 )
		{
			fprintf(stdout, "\n========================================\n");
			fprintf(stdout, "DISCONNECTED with iret = %d\n", iret);
			fprintf(stdout, "========================================\n");
			break;
		}
		if ( strcmp(buffer, "quit\n") == 0 ) { close(clnt_fd); break; }
		// save file
		else if (strncmp(buffer, "FILE:", 5) == 0) 
		{
			last_packge = time(NULL);

			receiving = 1;
			save_file(buffer, sizeof(buffer), clnt_fd); 
			memset(buffer, 0, sizeof(buffer));
		}
		else
		{
			printf("\n%s: %s\n", ip, buffer);
			fflush(stdout);
			memset(buffer, 0, sizeof(buffer));
		}
	}
}

void save_file(const char* buffer, size_t size, int clnt_fd)
{
	// buffer declaration
	char file_name[1024];	
	char package_count_str[1024];
	char package_total_num_str[1024];
	static size_t package_count = 0;
	int package_num = 0;
	
	// breakpoint resume
	static char package_status_initialized = 0;
	if (!package_status_initialized) 
	{
		memset(package_status, NOT_RECEIVED, sizeof(package_status));
		package_status_initialized = 1;
	}
	
	// save data
	sscanf(buffer, "FILE: %s - package %s of total %s:", file_name, package_count_str, package_total_num_str);
	strcat(file_name, "_server");
	package_num = atoi(package_count_str);
	package_total_num = atoi(package_total_num_str);
	
	// printf("filename: %s\npackage_str: %zu\n", file_name, package_count);
	printf("====================\n");
	printf("FILE NAME:\t%s\n", file_name);
	printf("PACKAGE NUMBER:\t%d\n", package_num);
	printf("TOTAL NUMBER:\t%zu\n", package_total_num);

	// receiving packages
	if (package_num >= 0)
	{	
		// save file
		package_count ++;
		FILE* file = fopen(file_name, "a+");
		fseek(file, 0, SEEK_END);
		fprintf(file, "%s", buffer + HED_SIZE );
		package_status[package_count] = RECEIVED;

		fclose(file);	
	}
	else
	{
		call_resume(package_status, 1024, package_total_num, clnt_fd);
	}
}

void call_resume(char* status, size_t length, size_t message_count, int clnt_fd)
{
	// breakpoint resume
	size_t ite = 0;
	char all_received = 1;
	for (ite = 1; ite <= message_count; ite++)
	{
		if (status[ite] == NOT_RECEIVED)
		{
			all_received = 0;
			char temp[1024];
			
			sprintf(temp, "FILE: packag %zu NOT_RECEIVED", ite);
			printf("FILE: packag %zu NOT_RECEIVED\n", ite);
			send(clnt_fd, temp, strlen(temp), 0);
		}
	}
	if (all_received) 
	{
		printf("====================\n");
		printf("ALL PACKAGES RECEIVED!\n");
		printf("====================\n");
		receiving = 0;
	}
}
