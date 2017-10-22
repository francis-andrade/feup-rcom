#include "datalink.h"

struct termios oldtio, newtio;

int open_port(char* destination){
    int fd = open(destination, O_RDWR | O_NOCTTY );
    if (fd <0) { perror(destination); exit(-1); }

	//saving current port settings
    if ( tcgetattr(fd,&oldtio) == -1) {
      perror("tcgetattr");
      exit(-1);
    }

	//configurating newtio and setting input mode
	bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 chars received */ //old: 5

	//clearing buffer
    tcflush(fd, TCIOFLUSH);

	//applying changes
    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

	return fd;
}

void byte_stuff(unsigned char** buf, int size){
    int i, newsize = size;
    unsigned char * res;


    for (i = 0; i < size; i++){
      if (((*buf)[i] == FLAG) || ((*buf)[i] == ESCAPE))
        newsize++;
    }

    res = (unsigned char*) malloc(newsize);
    int j = 0;
    for (i = 0; i < size; i++){
      if ((*buf)[i] == FLAG){
        res[j] = ESCAPE;
        j++;
        res[j] = FLAG_E;
      } else if ((*buf)[i] == ESCAPE){
        res[j] = ESCAPE;
        j++;
        res[j] = ESCAPE_E;
      } else res[j] = (*buf)[i];
      ++j;
    }

    free(*buf);
    *buf = res;
  }

int byte_destuff(unsigned char** buf, int size){
  unsigned char * res;
  int i, newsize = size;
  int j = 0;


  for (i = 0; i < size; i++){
    if((*buf)[i] == ESCAPE){
      i++;
      if ((*buf)[i] == ESCAPE_E)
        (*buf)[j] = ESCAPE;
      else if ((*buf)[i] == FLAG_E)
        (*buf)[j] = FLAG;
      else return -1;
    }
    j++;
  }

  *buf = (unsigned char *) realloc(*buf, j);
  return j;
}

int llopen(unsigned char* port, int status){

}

int llclose(int fd){

}

int llwrite(int fd, char* buffer, int length){

}

int llread(int fd, char* buffer){

}
