// includes
#include <fcntl.h> 
#include "url.h"
#include "sockets.h"


//
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


//
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


//
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


//
void print_usage(){
    printf("Usage: download ftp://[<user>:<password>@]<host>/<url-path>\n");
    printf("E.g: download ftp://ei12107:pass123@tom.fe.up.pt/xenotron.jpg\n");
    printf("E.g: download ftp://speedtest.tele2.net/512KB.zip\n");
}


//
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