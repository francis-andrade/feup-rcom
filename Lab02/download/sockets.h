#pragma once

// includes
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include "utils.h"

//defines
#define MAX_SIZE 256

//
int create_socket (char *ip, int port);
int get_code(char * buffer);
int read_code_socket(int socket_fd);
int write_socket(int socket_fd, char * msg);