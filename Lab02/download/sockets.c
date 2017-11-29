#include "sockets.h"


int create_socket (char *ip, int port) {
	// server address handling
	struct sockaddr_in server_addr;
	bzero((char*) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip); /*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port); /*server TCP port must be network byte ordered */

	// attempt to open socket
	int socket_fd;
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0))<0) {
		printf("Failed to open socket.\n");
		return -1;
	}

	// attempt connection to server
	if (connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr))<0) {
		printf("Failed to connect to server via new socket.\n");
		return -2;
	}

	return socket_fd;
}


int get_code(char * buffer){
	// filter retcode (all chars up until 'space' char)
	char * pos_space = strchr(buffer, ' ');
	int len = pos_space - buffer;
	buffer[len] = 0;
	// parse and retrieve retcode
	return atoi(buffer);
}

int read_code_socket(int socket_fd){
	// attempt to read from socket
	char buffer[MAX_SIZE];
	bzero(buffer,MAX_SIZE);
	int len = read(socket_fd, buffer, MAX_SIZE);
	if (len<=0){
		printf("Failed to read from socket.");
		return -2;
	}
	buffer[len] = '\0';
	// get retcode
	return get_code(buffer);
}


int write_socket(int socket_fd, char * msg){
	char * msg2 = concat(msg, "\n");
	int len = strlen(msg2);
	int retcode = write(socket_fd, msg2, len);
	free(msg2);
	if (len == retcode)
		return 0;
	else {
		return -1;
	}
}