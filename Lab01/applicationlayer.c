#include "applicationlayer.h"
#include "datalink.h"
#include <stdio.h>  /* printf */
#include <stdlib.h> /* fopen, fseek, ... */
#include <string.h>

int sender(Applicationlayer app, const char* port, const char* filename){
  int i, j;
  // open serial bus fd
  int fd = llopen(port, SENDER);

  //open file (e.g. penguin)
  FILE *fp = fopen(filename, "rb");

  // get the filesize
  fseek(fp, 0, SEEK_END);
  size_t filesize = ftell(fp);
  rewind(fp);

  // get chunk_size
  size_t chunk_size = MAX_SIZE-4; //TODO is it -4 ?

  // init buffer, packet arrays
  unsigned char buffer[chunk_size];
  unsigned char packet[MAX_SIZE];

  // send START packet
  i = 0;
  // 1st TLV: filesize
    packet[i++] = AL_C_START; // control = 1

    packet[i++] = AL_T_SIZE;
    packet[i++] = sizeof(size_t);
    temp = filesize;

    for (j=0; j<sizeof(size_t); ++j, ++i){
      packet[i+j] = temp%256;
      temp = temp/256;
    }
  // 2nd TLV: filename
    packet[i++] = AL_T_NAME;
    packet[i++] = strlen(filename);

    for (j=0; j<strlen(filename); ++j, ++i){
      packet[i+j] = filename[j];
    }

  // send all other packets
  int sequence_no; // TODO maybe from LinkLayer Struct
  while (fread(buffer, chunk_size, 1, fp) > 0){ // read a chunk of the file
    // packet header
    packet[0] = AL_C_DATA; // control = 1
    packet[1] = sequence_no % 256; // sequencia %255
    packet[2] = chunk_size / 256 // l2
    packet[3] = chunk_size % 256 // l1
    // packet body
    j=4;
    for (i=0; i<chunk_size; ++i)
      packet[j] = buffer[i];
    // attempt to write
    int res;
    do {
      res = llwrite(fd);
      // timeouts are accounted for within llwrite, using timeout_flag
    } while (res<0);
  }

  // send END packet
  //TODO
}

int receiver(Applicationlayer app){
    int fd = llopen(port, RECEIVER);
}
