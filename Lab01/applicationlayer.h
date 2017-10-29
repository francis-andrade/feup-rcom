#ifndef APPLICATIONLAYER_H
#define APPLICATIONLAYER_H

#include <stdio.h>  /* printf */
#include <stdlib.h> /* fopen, fseek, ... */
#include <string.h>
#include <stddef.h>

#define AL_C_DATA 1
#define AL_C_START 2
#define AL_C_END 3

#define AL_T_SIZE 0
#define AL_T_NAME 1

typedef enum{
  SENDER, RECEIVER
} Mode;

typedef struct {
  int filedescriptor;
  int status;
} Applicationlayer;


int sender(Applicationlayer app, const char* port, const char* filename);
int receiver(Applicationlayer app, const char* port);

int create_control_packet(unsigned char * packet, const char* filename, unsigned char control, size_t filesize); 
int create_data_packet(int sequence_no, int chunk_size, unsigned char *packet, unsigned char *buffer); 

#endif
