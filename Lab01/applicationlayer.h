#ifndef APPLICATIONLAYER_H
#define APPLICATIONLAYER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#define AL_C_DATA 1
#define AL_C_START 2
#define AL_C_END 3
#define AL_T_SIZE 0
#define AL_T_NAME 1
//#define CHUNK_SIZE 100
#define TMP_FILENAME "receiving.tmp"

typedef enum{
  SENDER, RECEIVER
} Mode;

typedef struct {
  Mode mode; 	 
  char* port; 	// serialbus device path
  char* filename;
  size_t filesize;
  int fd; 		// file descriptor
} ApplicationLayer;

int sender(const char* port, const char* filename);
int receiver(const char* port);
int create_control_packet(unsigned char * packet, const char* filename, const unsigned char control, size_t filesize);
int create_data_packet(int sequence_no, unsigned char *chunk, size_t chunk_size, unsigned char *packet);
int translate_baudrate(int baudr);

#endif
