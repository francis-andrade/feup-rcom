#include "url.h"


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
	char *next = url->path;
	while((next = strchr(next, '/'))){
		next += 1;
		filename = next;
	}
	strcpy(url->file, filename);

	return 0;
}



int getip (url_struct * url) {
	struct hostent *h;
	if ((h = gethostbyname(url->host)) == 0) {
		return -1;
	}

	strcpy(url->ip, inet_ntoa(*((struct in_addr *)h->h_addr)));
	return 0;
}