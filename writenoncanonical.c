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

volatile int STOP=FALSE;

void alarm_handle(int sig) {
    printf("Timeout (awaiting UA)\n");
    STOP = TRUE;
    deinit();
    exit(0);
}	

void deinit(){	
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    // close
	close(fd);
}

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    unsigned char buf[255];
    int i, sum = 0, speed = 0;
    
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

	
    // get to buf
    //gets(buf);
 	//int len = strlen(buf);
	//printf("Inputted: %s:%d\n", buf, len);
	//++len; // \0 included


	//send a SET frame
    unsigned char set[5];
    set[0] = FLAG;
    set[1] = A;
    set[2] = SET; // this is C
    set[3] = set[1]^set[2];	
    set[4] = FLAG;
    res = write(fd,buf,5);   
	printf("Sent SET: %c,%c,%c,%c,%c:%d\n", set[0],set[1],set[2],set[3],set[4], res);
	sleep(3);
 	
 	//create alarm
    signal(SIGALRM, alarm_handle);
    alarm(3);

    //wait for a UA
    int len = 0;
    while(!STOP){
    	res = read(fd,& buf[len],1);
    	// if (buf[len]==FLAG) STOP=TRUE;
    	++len;
    }
    alarm(0); //If seconds is zero, any pending alarm is canceled.
	printf("Received: %s:%d\n", buf, len);



	sleep(3);
	deinit();
    return 0;
}
