/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x7e
#define A 0x03
#define SET 0x03
#define UA 0x07

int STOP=FALSE;
int fd;
struct termios oldtio,newtio;
unsigned char set[5];
unsigned char ua[5];

void init_SET(){
  //set the SET frame
  set[0] = FLAG;
  set[1] = A;
  set[2] = SET; // this is C
  set[3] = set[1]^set[2]; 
  set[4] = FLAG;
}

void init_UA(){
  //set the UA frame
  ua[0] = FLAG;
  ua[1] = A;
  ua[2] = UA; // this is C
  ua[3] = ua[1]^ua[2]; 
  ua[4] = FLAG;
}


void llclose(){
  // reset TC to old config
  if (tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }
  // close
  close(fd);
  exit(0);
}

int nSETs = 0;
void send_SET(){
  int res = write(fd,set,5); 
  printf("Sent SET: %x,%x,%x,%x,%x:%d\n", set[0],set[1],set[2],set[3],set[4], res);
  ++nSETs;
  sleep(3);
}

void alarm_open(int sig) {
  printf("Timeout (awaiting UA)\n");
  if (nSETs < 3){
    printf("Resending SET\n");
    send_SET();
    alarm(3);
  }
  else {
    printf("Timedout 3x. Terminating...\n");
    deinit();
  }
}	

void alarm_write(int sig) {
	printf("Timeout (awaiting RR) \n");
}

void init_termios(char device[]){ //device e.g.: /dev/ttyS0
  //open file descriptor
  fd = open(device, O_RDWR | O_NOCTTY );
  if (fd <0) {
    perror(device); 
    exit(-1); 
  }

  // get old TC config
  if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }

  // create newtio
  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag      = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag      = IGNPAR;
  newtio.c_oflag      = 0;
  newtio.c_lflag      = 0;   // set input mode (non-canonical, no echo,...)
  newtio.c_cc[VTIME]  = 0;   // inter-character timer unused 
  newtio.c_cc[VMIN]   = 1;   // blocking read until 1 chars received 

  // set new TC config
  tcflush(fd, TCIOFLUSH);
  if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }
  printf("New termios structure set\n");
}

int llopen(){
  //send first SET
  send_SET();

  //create alarm
  signal(SIGALRM, alarm_open);
  alarm(3);

  //wait for a UA
  unsigned char buf[255];
  int len = 0;
  int ua_state = 0;
  int res;
  while(!STOP){
    res = read(fd,buf+len,1);
    printf("Received[%d] = %x\n", len, buf[len]);
    if(res != 0) {
      switch(ua_state){
        case 0: //flag
          if (buf[len] == ua[0])
            ua_state = 1;
          else 
            ua_state = 0;
          break;
        case 1: //a 
          if (buf[len] == ua[1]) 
            ua_state = 2;
          else if (buf[len] == ua[0])
            ua_state = 1;
          else 
            ua_state = 0;
          break;
        case 2: //c
          if (buf[len] == ua[2])
            ua_state = 3;
          else if (buf[len] == ua[0])
            ua_state = 1;
          else 
            ua_state = 0;
          break;
        case 3: //bcc
          if (buf[len] == ua[3])
            ua_state = 4;
          else if (buf[len] == ua[0])
            ua_state = 1;
          else 
            ua_state = 0;
          break;
        case 4: //flag
          if (buf[len] == ua[4]){
            ua_state = 5;
            STOP = TRUE;
          } 
          else if (buf[len] == ua[0])
            ua_state = 1; 
          else 
            ua_state = 0;
          break;
      }
      if (ua_state == 0)
        len = 0;
    }
    ++len;
  }

  //We have received UA
  alarm(0); //If seconds is zero, any pending alarm is canceled.
  printf("Received UA: %x,%x,%x,%x,%x:%d\n", ua[0],ua[1],ua[2],ua[3],ua[4], res);
  return 1;
}

int llwrite(int fd, char * buffer, int length){
	int tries=0;
	while(tries<3){
		
	}
}


int main(int argc, char** argv){
  if ( (argc < 2) || 
    ((strcmp("/dev/ttyS0", argv[1])!=0) && 
     (strcmp("/dev/ttyS1", argv[1])!=0) )) {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }

  // initialize
  init_termios(argv[1]);
  init_SET();
  init_UA();
 
  // atempt to open
  if (!llopen())
    exit(-1);

  printf("llopen connected.");

  //terminate
  llclose();
  return 0;
}
