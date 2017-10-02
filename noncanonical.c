/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x7e
#define A 0x03
#define C_SET 0x03

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    unsigned char buf[255];

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
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 chars received */ //old: 5


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
	

	unsigned char c;
	unsigned char SET[5];
	int state = 0;
	
	while (state != 5){
printf("entered state machine\n");
		read(fd, &c, 1);	
		printf("%x\n", c);	
		switch(state){
			case 0:
				if(c == FLAG){
					state = 1;
					SET[0] = c;}
				else state = 0;
			break;
			case 1:
				if (c == FLAG) state = 1;
				else if (c == A){
					state = 2; 
					SET[1] = c;}
				else state = 0;
			break;
			case 2: 
				if (c == FLAG) state = 1;
				else if (c = C_SET){
					state = 3;
					SET[2] = c;}
				else state = 0;
			break;
			case 3: 
				if (c == FLAG) state = 1;
				else if (c == SET[1]^SET[2]){
					state = 4;
					SET[3] = c;}
				else state = 0; 
			break;
			case 4: 
				if (c == FLAG){
					state = 5;
					SET[4] = c;}
				else state = 0;
			break;		
		}	
	}

	printf("Read message successfully:\n  -SET[0]=%X\n  -SET[1]=%X\n  -SET[2]=%X\n  -SET[3]=%X\n  -SET[4]=%X\n",
		SET[0],SET[1],SET[2],SET[3],SET[4]);

	printf("Sending acknowledgement message...\n");	
		
	//building acknowledgement message
	SET[0]=FLAG;
	SET[1]=A;
	SET[2]=C_SET;
	SET[3]=SET[1]^SET[2];
	SET[4]=FLAG;
	res = write(fd, SET, 5);

	printf("Sent over %d bytes\n",res);


	/*
		// read the message byte by byte
	int i = 0;
	while (STOP==FALSE) {       		//loop for input
	   	res = read(fd,buf + i,1);   	//returns after 1 chars have been input //old: 255	
	    if (buf[i] == 0) STOP = TRUE;	
	  	++i;
    }
    printf("Received: %s:%d\n", buf, i);


		// sent the message back    
    res = write(fd, buf, i);
    printf("Sent back: %s:%d\n", buf, res);
	*/
	
	sleep(3);
    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
