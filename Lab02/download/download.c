#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_SIZE 256

char* concat(const char *s1, const char *s2)
{
    char *result = (char*)malloc(strlen(s1)+strlen(s2)+1);//+1 for the null-terminator
    //in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}



typedef struct {
	// phase 1
	char full[MAX_SIZE];
	// phase 2
	char user[MAX_SIZE];
	char pass[MAX_SIZE];
	char host[MAX_SIZE];
	char path[MAX_SIZE];
	char file[MAX_SIZE];
	// phase 3
	char ip[MAX_SIZE];
} url_struct;



int parse_full_url (const char * full_url, url_struct * url) {
	// init struct
	bzero(url->full, MAX_SIZE);
	bzero(url->user, MAX_SIZE);
	bzero(url->pass, MAX_SIZE);
	bzero(url->host, MAX_SIZE);
	bzero(url->path, MAX_SIZE);
	bzero(url->ip, MAX_SIZE);

	// get full url 
	strcpy(url->full, full_url);

	// does the url conform to the 'ftp://' prefix?
	if (strncmp("ftp://", full_url, 6)!=0){
		printf("No prefix 'ftp://' found.");
		return -1;
	}
	
	// does the url contain @? where?
	char * pos_at = strchr((char*)full_url, '@');
	char * pos_host = 0;
	// yep
	if (pos_at){
		// where is the colon?
		char * pos_colon = strchr((char*)(full_url)+6, ':');
		if (pos_colon==0){
			printf("No colon separating user:pass found.");
			return -2;
		}
		// get user
		char * pos_user = (char*)(full_url)+6;
		int len_user = pos_colon - pos_user;
		strncpy(url->user, pos_user, len_user);
		url->user[len_user] = 0;
		// get pass
		char * pos_pass = pos_colon + 1;
		int len_pass = pos_at - pos_pass;
		strncpy(url->pass, pos_pass, len_pass);
		url->pass[len_pass] = 0;
		// set pos_host
		pos_host = pos_at+1;
	}
	// nope
	else {
		// fabricate anonymous:anonymous, according to RFC 1738
		// this is necessary for those URLs in which user:pass is not needed
		// for more info, consult section 3.2.1 of https://www.rfc-editor.org/rfc/rfc1738.txt
        strcpy(url->user, "anonymous");
        strcpy(url->pass, "anonymous");
        // set pos_host
        pos_host = (char*)(full_url)+6;
	}

	// where is the slash?
	char * pos_slash = strchr(pos_host, '/');
	
	// get host
	strncpy(url->host, pos_host, (pos_slash - pos_host));

	// get path 
	strcpy(url->path, pos_slash+1);

	// get filename
	char *filename = url->path;
	char *next;
	while (next = strchr(url->path, '/'))
		filename = next;
	strcpy(url->file, filename);

	return 0;
}


int getip (url_struct * url) {
	struct hostent *h;
	if ((h = gethostbyname(url->host)) == 0) {
		return 1;
	}

	strcpy(url->ip, inet_ntoa(*((struct in_addr *)h->h_addr)));
	return 0;
}


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



int login (int socket_fd, url_struct* url) {
	int retcode;

	// user
	char* msg_user = concat("user ",url->user);
	if (write_socket(socket_fd, msg_user)<0){
		return -1;
	}
	free(msg_user);
	if ((retcode = read_code_socket(socket_fd))!=331){
    	printf("Expected code 331. Got %d instead.\n", retcode);
		return -2;
	}

	// pass
	char* msg_pass = concat("pass ",url->pass);
	if (write_socket(socket_fd, msg_pass)<0){
		return -3;
	}
	free(msg_pass);
	if ((retcode = read_code_socket(socket_fd))!=230){
    	printf("Expected code 230. Got %d instead.\n", retcode);
		return -4;
	}

	// success!
	return 0;
}



int pasv (int socket_fd, char * ip, int * port) {
	// write pasv
	char msg_pasv[] = "pasv\n";
	write_socket(socket_fd, msg_pasv);

	// attempt to read from socket
	char buffer[MAX_SIZE];
	bzero(buffer,MAX_SIZE);
	int len = read(socket_fd, buffer, MAX_SIZE);
	if (len<=0){
		printf("Failed to read from socket.");
		return -1;
	}
	buffer[len] = '\0';
	// printf("msg: %s\n",buffer);

	// check retcode
	if (strncmp(buffer, "227", 3)!=0){
    	printf("Expected code 227. Got %d instead.\n", get_code(buffer));
		return -2;
	}

	// get ip
	char * pos_prev = strchr(buffer, '(')+1;
	char * pos_next;
	for (int i=0; i<4; ++i){
		pos_next = strchr(buffer, ',');
		pos_next[0] = '.';
	}
	len = pos_next - pos_prev;
	strncpy(ip, pos_prev, len);
	ip[len] = 0;

	// get port's first factor string
	char factor1[4];
	pos_prev = pos_next+1;
	pos_next = strchr(buffer, ',');
	// cut it out
	len = pos_next - pos_prev;
	strncpy(factor1, pos_prev, len);
	factor1[len] = 0;

	// get port's seconds factor string
	char factor0[4];
	pos_prev = pos_next+1;
	pos_next = strchr(buffer, ')');
	// cut it out
	len = pos_next - pos_prev;
	strncpy(factor0, pos_prev, len);
	factor0[len] = 0;

	// finish!
	*port = atoi(factor1)*256 + atoi(factor0);
	return 0;
}



int download(int cmd_socket, int data_socket, url_struct * url) { //TODO
	// 
	char* msg_retr = concat("retr ",url->path);
	if (write_socket(cmd_socket, msg_retr)<0){
		return -1;
	}
	free(msg_retr);
	int retcode;
	if ((retcode = read_code_socket(cmd_socket))!=150){
    	printf("Expected code 150. Got %d instead.\n", retcode);
		return -2;
	}

	// get file descriptor
	int fd = creat(url->file, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	// write into file
	int len;
	char buffer[MAX_SIZE];
	while ((len = read(data_socket, buffer, MAX_SIZE)) >= 1) 
		write(fd, buffer, len);
	close(fd);
	
	//return 
	return 0;
}



void print_usage(){
    printf("Usage: download ftp://[<user>:<password>@]<host>/<url-path>\n");
    printf("E.g: download ftp://ei12107:pass123@tom.fe.up.pt/xenotron.jpg\n");
    printf("E.g: download ftp://speedtest.tele2.net/512KB.zip\n");
}



int main (int argc, char *argv[]) {
    int retcode;
	
	// application requires 1 param
	if (argc != 2) {
		printf("%s requires 1 parameter to run.\n",argv[0]);
		print_usage();
        return 1;
	}

	// parse argv1 into all url required parts
    url_struct url;
    if ((retcode = parse_full_url(argv[1], &url))!=0){
    	printf("Failed to parse '%s' into all required URL elements. Error #%d\n", argv[1], retcode);
		print_usage();
		return 2;
    }
    printf("USER=%s\nHOST=%s\nPATH=%s\nFILE=%s\n", url.user, url.host, url.path, url.file);

    // transform host into a ip
    if ((retcode = getip(&url))!=0){
    	printf("Failed to transform host into an IP. Error #%d\n", retcode);
		return 3;
    }
    printf("IP=%s\n", url.ip);

    // create command socket
    int cmd_socket = 0;
    if ((cmd_socket = create_socket(url.ip, 21))<=0){
    	printf("Failed to create command socket on %s:21 host into an IP. Error #%d\n", url.ip, cmd_socket);
		return 4;
    }
    if ((retcode = read_code_socket(cmd_socket))!=220){
    	printf("Expected code 220. Got %d instead.\n", retcode);
	    close(cmd_socket);
		return 5;
    }

    // login
    if ((retcode = login(cmd_socket, &url))!=0){
    	printf("Failed to login. Error #%d\n", retcode);
	    close(cmd_socket);
		return 6;
    }

    // query and get port for data socket
    char data_ip[MAX_SIZE];
    int data_port;
    if ((retcode = pasv(cmd_socket, data_ip, &data_port))!=0){
    	printf("Failed to pasv. Error #%d\n", retcode);
	    close(cmd_socket);
		return 7;
    }

    // create another socket to receive file
    int data_socket=0;
    if ((data_socket = create_socket(data_ip, data_port))<=0){
    	printf("Failed to create data socket to receive file. Error #%d\n", data_socket);
	    close(cmd_socket);
		return 8;
    }

    // download!
    if ((retcode = download(cmd_socket, data_socket, &url))!=0){
    	printf("Failed to create another socket to receive file. Error #%d\n", retcode);
	    close(cmd_socket);
		close(data_socket);
		return 9;
    }
	printf("Sucessfully downloaded file!\n");
    
	// terminate
    printf("Closing sockets and exiting...\n");
    close(cmd_socket);
	close(data_socket);
	return 0;
}