#ifndef DATALINK_H
#define DATALINK_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_SIZE 255
#define BAUDRATE B38400
#define TIMEOUT_DURATION 3
#define TIMEOUT_TRIES 4

//  Frames
#define ESCAPE 0x7d
#define ESCAPE_E 0x5d
#define FLAG_E 0x5e
#define FLAG 0x7e
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07
#define C_DISC 0x0B
#define C_RR0 0x05
#define C_REJ0 0x01
#define C_RR1 0x85
#define C_REJ1 0x81
#define C_DATA1 0x40
#define C_DATA0 0x00

//  Packets
#define PC_CONT 0x01
#define PC_START 0x02
#define PC_END 0x03

typedef struct{
  char port[20];
  int baudrate;
  unsigned int sequenceNumber;
  unsigned int timeout;
  unsigned int numTransmissions;
  char frame[MAX_SIZE];
} linklayer;

extern linklayer * ll;

typedef enum{
  S_START, S_FLAG, S_ADDRESS, S_CONTROL, S_BCC1,  S_DN, S_END
} State;

typedef struct{
  int success;
  unsigned char address;
  unsigned char control;
  unsigned char* data;
  int size;
} State_Frame;

typedef void (*func_ptr)(void);

int byte_stuff(unsigned char** buf, int size);
int byte_destuff(unsigned char** buf, int size);
int send_frame(unsigned char* frame, int fd);
int open_port(const char* destination);
int close_port(int fd);
int llopen(const char* port, int status);
int llclose(int fd, int status);
int llwrite(int fd, unsigned char* buffer, int length);
int llread(int fd, unsigned char* buffer);
State_Frame state_machine(int fd);
int build_frame_sup(unsigned char address, unsigned char control, unsigned char * FRAME);
int build_frame_data(unsigned char address, unsigned char control, unsigned char ** FRAME, unsigned char * PACKET, int length);
unsigned char create_BCC(unsigned char * PACKET, int size);

#endif
