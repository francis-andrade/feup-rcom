#include <termios.h>
#include "utils.h"
#include "applicationlayer.h"

#define MAX_SIZE 255;

struct linklayer{
  char port[20];
  int baudrate;
  unsigned int sequenceNumber;
  unsigned int timeout;
  unsigned int numTransmissions;
  char frame[MAX_SIZE];
}

void byte_stuff(unsigned char** buf, int size);
void byte_destuff(unsigned char** buf, int size);
int send_frame(unsigned char* frame, int fd);
int open_port(char* destination);
int llopen(unsigned char* port, int status);
int llclose(int fd);
int llwrite(int fd, char* buffer, int length);
int llread(int fd, char* buffer);
