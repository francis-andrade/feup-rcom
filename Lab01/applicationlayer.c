#include "applicationlayer.h"

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
  create_control_packet(packet, filename, AL_C_START, filesize);
  //TODO send packet

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

  create_control_packet(packet, filename, AL_C_END, filesize);
  //TODO send packet
}

int create_control_packet(unsigned char * packet, const char* filename, const char control, size_t filesize){
  int i = 0, j;
  packet[i++] = control;

  //1st TLV - file size
  packet[i++] = AL_T_SIZE;
  packet[i++] = sizeof(size_t);
  size_t temp = filesize;

  for (j=0; j<sizeof(size_t); ++j, ++i){
    packet[i+j] = temp%256;//TODO isto nao vai dar borrada? nao devia ser packet[i] apenas?
    temp = temp/256;
  }

  //2nd TLV - filename
  packet[i++] = AL_T_NAME;
  packet[i++] = strlen(filename);

  for (j=0; j<strlen(filename); ++j, ++i){
    packet[i+j] = filename[j];//TODO isto tambem nao vai dar erro?
  }

  return 0;
}

int receiver(Applicationlayer app){
    int fd = llopen(port, RECEIVER);
}
