#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define BUF_SIZE	1024	// size of buffer to receive data
#define HED_SIZE	100		// size of header in data
#define RECEIVED		1
#define NOT_RECEIVED	0

char receiving = 0;						// if the server is receiving file
char package_status[65536];				// records whether the package corresponding to the index is received
char package_status_initialized = 0;	// record if the package_status array is initialized
size_t package_total_num;				// the total number of packages

void error_handling(char* message);

// service function calls my_send & my_recv
void service(const char* ip, int clnt_fd);
// send message to client
void my_send(const char* ip, int clnt_fd);
// receive message from client
void my_recv(const char* ip, int clnt_fd);
// save the file from client
void save_file(const char* buffer, size_t size, int clnt_fd);
// notify the client of package loss
void call_resume(char* status, size_t message_count, int clnt_fd);

int main(int argc, char* argv[])
{
	if (argc != 2) 
	{
		fprintf(stdout, "Usage : %s <port>", argv[0]);
	}

	// listen socket
	int listen_fd;
	if ((listen_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) { error_handling("socket() error"); }

	// set server protocal 
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));

	// save the info of client
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_sz = sizeof(clnt_addr);

	// bind listen socket and server struct
	if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
	{ error_handling("bind() error"); }

	// start listening
	if (listen(listen_fd, 5) == -1)
	{ error_handling("listen() error"); }

	pid_t pid;
	while (1)
	{
		// accept connection and create socket with client
		int clnt_fd;
		if ((clnt_fd = accept(listen_fd, (struct sockaddr *)&clnt_addr, &clnt_addr_sz)) == -1)
		{ error_handling("accept() error"); }

		// print log on stdout
		fprintf(stdout, "========== Connection Acctped ==========\n");
		fprintf(stdout, "IP: %s \n", inet_ntoa(clnt_addr.sin_addr));
		fprintf(stdout, "port: %d \n", ntohs(clnt_addr.sin_port));
		fprintf(stdout, "========================================\n");

		pid = fork();

		if (pid == -1) { error_handling("fork() error"); }
		else if (pid == 0) 
		{
			// close listen_fd for it is only used in main process
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
	// create another process in the child process
	// one for send, another for receive
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
		// set buffer
		memset(buffer, 0, sizeof(buffer));
		// display an input prompt
		fprintf(stdout, "To %s : ", ip);
		// get message from stdin
		fgets(buffer, sizeof(buffer) - 1, stdin);

		// close connection
		if (strcmp(buffer, "quit\n") == 0) { close(clnt_fd); break; } 

		// if connection is broken
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
		// if is receiving file, check if there are lost packages
		if (receiving)	
		{
			call_resume(package_status, package_total_num, clnt_fd);
			// output message immediately
			fflush(stdout);
		}

		int iret;
		// if disconnected, output log
		if ((iret = recv(clnt_fd, buffer, sizeof(buffer), 0)) <= 0 )
		{
			fprintf(stdout, "\n========================================\n");
			fprintf(stdout, "DISCONNECTED with iret = %d\n", iret);
			fprintf(stdout, "========================================\n");
			break;
		}

		// close connection
		if ( strcmp(buffer, "quit\n") == 0 ) { close(clnt_fd); break; }
		// save file
		else if (strncmp(buffer, "FILE:", 5) == 0) 
		{
			// set flag
			receiving = 1;
			// let save_file to handle the package
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
	char file_name[1024];				// record the name of file	
	char package_count_str[1024];		// buffer to restore the serial number of the package 
	int package_num = 0;				// integer to restore the serial number of the package
	char package_total_num_str[1024];	// buffer to restore the total number of packages
	
	// breakpoint resume
	// if the status array is not initialized, initialize it
	if (!package_status_initialized) 
	{
		memset(package_status, NOT_RECEIVED, sizeof(package_status));
		package_status_initialized = 1;
	}
	
	// save data
	// extract infos from header string
	sscanf(buffer, "FILE: %s - package %s of total %s:", file_name, package_count_str, package_total_num_str);
	// append idenfitier after file name
	strcat(file_name, "_server");
	// save the data in the string to the integer
	package_num = atoi(package_count_str);
	package_total_num = atoi(package_total_num_str);
	
	// output log
	printf("====================\n");
	printf("FILE NAME:\t%s\n", file_name);
	printf("PACKAGE NUMBER:\t%d\n", package_num);
	printf("TOTAL NUMBER:\t%zu\n", package_total_num);

	// receiving packages
	if (package_num >= 0)
	{	
		// save file
		FILE* file = fopen(file_name, "a+");
		// move the file pointer to the end of file
		fseek(file, 0, SEEK_END);
		// append data
		fprintf(file, "%s", buffer + HED_SIZE );
		// set flag
		package_status[package_num] = RECEIVED;

		fclose(file);	
	}
	// if package_num < 0, it means server received the last package
	// start breakpoint resume
	else
	{
		call_resume(package_status, package_total_num, clnt_fd);
	}
}

void call_resume(char* status, size_t message_count, int clnt_fd)
{
	// breakpoint resume
	size_t ite = 0;
	// assume all packages are received
	char all_received = 1;


	for (ite = 1; ite <= message_count; ite++)
	{
		// if the flag of a package is not set
		if (status[ite] == NOT_RECEIVED)
		{
			all_received = 0;
			char temp[1024];
			
			sprintf(temp, "FILE: packag %zu NOT_RECEIVED", ite);
			printf("FILE: packag %zu NOT_RECEIVED\n", ite);
			// tell the client there is a package lost
			send(clnt_fd, temp, strlen(temp), 0);
		}
	}
	if (all_received) 
	{
		printf("====================\n");
		printf("ALL PACKAGES RECEIVED!\n");
		printf("====================\n");
		// we are not receiving packages now
		receiving = 0;
		// we need to re-initialize the array
		package_status_initialized = 0;
	}
}
