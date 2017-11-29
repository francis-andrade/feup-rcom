#pragma once

// includes
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

// defines
#define MAX_SIZE 256

//
typedef struct {
	// phase 1 - full_url initialized
	char full[MAX_SIZE];
	// phase 2 - full_url parsed into other strings
	char user[MAX_SIZE];
	char pass[MAX_SIZE];
	char host[MAX_SIZE];
	char path[MAX_SIZE];
	char file[MAX_SIZE];
	// phase 3 - host is converted into an IP
	char ip[MAX_SIZE];
} url_struct;

int parse_full_url (const char * full_url, url_struct * url);
int getip (url_struct * url);