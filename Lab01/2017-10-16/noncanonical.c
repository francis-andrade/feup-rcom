/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x7e
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07
#define C_DISC 0x0B
#define C_RR0 0x05
#define C_REJ0 0x01
#define C_RR1 0x85
#define C_REJ1 0x81

volatile int STOP=FALSE;
int fd, res;
struct termios oldtio,newtio;
char* destination;

int open_port(){
	fd = open(destination, O_RDWR | O_NOCTTY );
    if (fd <0) {perror(destination); exit(-1); }

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

	return 0;
}

int state_machine(unsigned char* SET){
	unsigned char ch;
	int state = 0;
	printf("Waiting for message...\n");
	while (state != 5){
		read(fd, &ch, 1);
		switch(state){
			case 0:
				if(ch == FLAG){
					state = 1;
					SET[0] = ch;}
				else state = 0;
			break;
			case 1:
				if (ch == FLAG) state = 1;
				else if (ch == A){
					state = 2;
					SET[1] = ch;}
				else state = 0;
			break;
			case 2:
				if (ch == FLAG) state = 1;
				else if (ch == C_SET){
					state = 3;
					SET[2] = ch;}
				else state = 0;
			break;
			case 3:
				if (ch == FLAG) state = 1;
				else if (ch == (SET[1]^SET[2])){
					state = 4;
					SET[3] = ch;}
				else state = 0;
			break;
			case 4:
				if (ch == FLAG){
					state = 5;
					SET[4] = ch;}
				else state = 0;
			break;
		}
	}

	return 0;
}

int send_supervision(unsigned char* SET){
	int r;
	SET[0]=FLAG;
	SET[1]=A;
	SET[2]=C_UA;
	SET[3]=SET[1]^SET[2];
	SET[4]=FLAG;
	r = write(fd, SET, 5);
	return r;
}

int llopen(){
//opening serial port
    if (open_port() == 0) printf("New termios structure set\n");
	else exit(-1);

	while(1){

		//receiving connection message
		unsigned char SET[5];
		if (state_machine(SET) == 0){
			printf("Read message successfully:\n  -SET[0] = %X\n  -SET[1] = %X\n  -SET[2] = %X\n  -SET[3] = %X\n  -SET[4] = %X\n",
				SET[0],SET[1],SET[2],SET[3],SET[4]);
			break;
		}

		//receiving messages

		//sleep(3);

		printf("Sending acknowledgement message...\n");

		//building acknowledgement message and sending it
		res = send_supervision(SET);

		printf("Sent over %d bytes\n",res);
	}
}

int main(int argc, char** argv){

  unsigned char buf[255];

  if ( (argc < 2) ||
  	((strcmp("/dev/ttyS0", argv[1])!=0) &&
  		(strcmp("/dev/ttyS1", argv[1])!=0) )) {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }

	destination = argv[1];

	llopen();

	sleep(3);
  tcsetattr(fd,TCSANOW,&oldtio);
	close(fd);
  return 0;
}
