#include "applicationlayer.h"

int sender(Applicationlayer app, const char* port, const char* filename){//TODO como e suposto o main mandar o port?
  int i, j, res, fd;
  size_t filesize, chunk_size;

  // open serial bus fd
  fd = llopen(port, SENDER);

  //open file (e.g. penguin)
  FILE *fp = fopen(filename, "rb");

  // get the filesize and chunk size
  fseek(fp, 0, SEEK_END);
  filesize = ftell(fp);
  rewind(fp);
  chunk_size = MAX_SIZE-4; //TODO is it -4 ?

  // init buffer, packet arrays
  unsigned char buffer[chunk_size];
  unsigned char packet[MAX_SIZE];

  // send START packet (timeouts are accounted for within llwrite, using timeout_flag)
  create_control_packet(packet, filename, AL_C_START, filesize);
  do {
    res = llwrite(packet, fd);
  } while (res<0);

  // send all other packets
  int sequence_no; // TODO maybe from LinkLayer Struct
  while (fread(buffer, chunk_size, 1, fp) > 0){ // read a chunk of the file
    create_data_packet(sequence_no, chunk_size, packet, buffer)
    do {
      res = llwrite(packet, fd);       
    } while (res<0);
  }
  
  //send END packet
  create_control_packet(packet, filename, AL_C_END, filesize);
  do {
    res = llwrite(packet, fd);
  } while (res<0);

  return 0;
}

int receiver(Applicationlayer app){
  int fd = llopen(port, RECEIVER);

  

}

int create_control_packet(unsigned char * packet, const char* filename, const char control, size_t filesize){
  int i = 0, j;
  packet[i++] = control;

  //1st TLV - file size
  packet[i++] = AL_T_SIZE;
  packet[i++] = sizeof(size_t);
  size_t temp = filesize;

  for (j=0; j<sizeof(size_t); ++j, ++i){
    packet[i+j] = temp%256;//TODO: isto nao vai dar borrada? nao devia ser packet[i] apenas?
    temp = temp/256;
  }

  //2nd TLV - filename
  packet[i++] = AL_T_NAME;
  packet[i++] = strlen(filename);

  for (j=0; j<strlen(filename); ++j, ++i){
    packet[i+j] = filename[j];//TODO: isto tambem nao vai dar erro?
  }

  return 0;
}

int create_data_packet(int sequence_no, int chunk_size, unsigned char *packet, unsigned char *buffer){
  // packet header
  packet[0] = AL_C_DATA;         // control = 1
  packet[1] = sequence_no % 256; // sequencia %255
  packet[2] = chunk_size / 256;  // l2
  packet[3] = chunk_size % 256;  // l1

  // packet body
  int j = 4;
  for (i = 0; i < chunk_size; ++i)
    packet[j] = buffer[i];
}
