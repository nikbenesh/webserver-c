#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "network_lib.h"

#define PORT 3600
#define WEBROOT "./webroot"
#define LOGFILE "smallweb.log"

// Это программа работает автономно как локальный веб-сервер
// Чтобы отследить ее работу, введите в адресной строке браузера 'localhost:3600'
// На отобразившейся странице вы увидите содержание HTML-файлов из папки webroot

int logfd, sockfd;

void handle_connection(int sockfd, struct sockaddr_in *client_addr, int logfd);
void handle_shutdown(int signal);
int get_file_size(int fd);
void timestamp(int logfd);
void close_connection(char log_buffer[500], int sockfd, int logfd, int fd);
void fatal(char measage[500]);

int main(void) {
	int new_sockfd, yes=1, recv_length=1, p;
	struct sockaddr_in host_addr, client_addr;
	char buffer[1024];
	socklen_t sin_size;

	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) fatal("in socket");

	if ( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		fatal("while setting socket");

	logfd = open(LOGFILE, O_CREAT|O_APPEND|O_RDWR, S_IRUSR|S_IWUSR);
	if (logfd == -1)
		fatal("opening log file");

	host_addr.sin_family = AF_INET;
	host_addr.sin_addr.s_addr = INADDR_ANY;
	host_addr.sin_port = htons(PORT);
	memset(&(host_addr.sin_zero), '\0', 8);

	if ( bind(sockfd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr) ) == -1)
		fatal("in bind");
	
	printf("Starting web server daemon...\n");
	p = fork();
	if(p == -1)
		fatal("forking to daemon process");
	else if (p == 0) {
		signal(SIGTERM, handle_shutdown);
		signal(SIGINT, handle_shutdown);
	
		timestamp(logfd);
		write(logfd, "Starting up...\n", 15);
	
		if (listen(sockfd, 5) == -1)
			fatal("while listening");
	
		while (1) {
			sin_size = sizeof(struct sockaddr_in);
			new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
	
			if (new_sockfd == -1) 
				fatal("accepting client");
	
			handle_connection(new_sockfd, &client_addr, logfd);
		}
	}
	else {
		close(sockfd);
		return 0;
	}
}


void handle_connection(int sockfd, struct sockaddr_in *client_addr, int logfd) {
	int fd, length, i, n;
	char *ptr, request[500], resource[500], log_buffer[500], buffer[100];

	length = recv_line(sockfd, (u_char *)&request);

	if (length == -1) {
		strcat(log_buffer, "error getting request\n");
		 close_connection(log_buffer, sockfd, logfd, -1000);
	}

	sprintf(log_buffer, "Got connection %s:%d '%s'\n", inet_ntoa(client_addr->sin_addr), 
		ntohs(client_addr->sin_port), request);
	
	ptr = strstr(request, "HTTP/");

	if (ptr == NULL) {
		strcat(log_buffer, "Non-HTTP request\n");
		close_connection(log_buffer, sockfd, logfd, -1000);
	}

	*ptr = 0;
	ptr = NULL;

	  if (strncmp(request, "GET ", 4) == 0)
		  ptr = request+4;

 	else if (strncmp(request, "HEAD ", 5) == 0)
		ptr = request+5;

	else {
		strcat(log_buffer, "Unknown HTTP request\n");
		 close_connection(log_buffer, sockfd, logfd, -1000);
	}

	strcpy(resource, WEBROOT);
	strcat(resource, ptr);

	n = strlen(resource)-2;
	if (resource[n+1] == ' ') 
		resource[n+1] = '\0';

	if (resource[n] == '/')
		strcat(resource, "index.html");

	sprintf(buffer, "\tOpening \'%s\'\t", resource);
	strcat(log_buffer, buffer);
	fd = open(resource, O_RDONLY, 0);

	if (fd == -1) {
		strcat(log_buffer, " 404 not found\n");
		send_string(sockfd, "HTTP/1.0 404 NOT FOUND\r\n", 0);
        send_string(sockfd, "Server: Tiny webserver\r\n\r\n", 0);
        send_string(sockfd, "<html><head><title>404 Not Found</title></head>", 0);
        send_string(sockfd, "<body><h1>URL not found</h1><br></body></html>\r\n", 0);
        close_connection(log_buffer, sockfd, logfd, -1000);
	}

	strcat(log_buffer, " 200 OK\n");
    send_string(sockfd, "HTTP/1.0 200 OK\r\n", 0);
    send_string(sockfd, "Server: Tiny webserver\r\n\r\n", 0);

    if (ptr == request+4) {
    	length = get_file_size(fd);
	
		if (length == -1) {
			strcat(log_buffer, " Error getting file size\n");
			close_connection(log_buffer, sockfd, logfd, fd);
		}
	
		ptr = (char *) malloc(length);
	
		if (ptr == NULL) {
			strcat(log_buffer, " Error allocating memory\n");
			close_connection(log_buffer, sockfd, logfd, fd);
		}
	
		read(fd, ptr, length);
		send_string(sockfd, ptr, length);
	}

	free(ptr);
	close_connection(log_buffer, sockfd, logfd, fd);
}

void close_connection(char log_buffer[500], int sockfd, int logfd, int fd) {
	shutdown(sockfd, SHUT_RDWR);
	timestamp(logfd);
	write(logfd, log_buffer, strlen(log_buffer));
	if (fd != -1000)
		close(fd);
}

void timestamp(int logfd) {
	struct tm *time_struct;
	time_t now;
	int len;
	char buffer[40];

	time(&now);
	time_struct = localtime((const time_t *)&now);
	len = strftime(buffer, 40, "%m/%d/%Y/ %H:%M:%S>", time_struct);
	write(logfd, buffer, len);
}

void handle_shutdown(int signal) {
	timestamp(logfd);
	write(logfd, "Shuting down...\n\n", 16);
	close(logfd);
	close(sockfd);
}

int get_file_size(int fd) {
	struct stat stat_struct;

	if (fstat(fd, &stat_struct) == -1)
		return -1;

	return stat_struct.st_size;
}

void fatal(char measage[500]) {
	printf("FATAL: %s\n", measage);
	exit(0);
}