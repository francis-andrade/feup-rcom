#include "utils.h"

int build_frame_sup(unsigned char address, unsigned char control, unsigned char * FRAME){
  FRAME = malloc(5);
  FRAME[0] = FLAG;
  FRAME[1] = address;
  FRAME[2] = control;
  FRAME[3] = FRAME[1] ^ FRAME[2];
  FRAME[4] = FLAG;
  return 5;
}

int build_frame_data(unsigned char address, unsigned char control, unsigned char * FRAME, unsigned char * PACKET){
  int p_size = 256*PACKET[2] + PACKET[3] + 4;
  int f_size = p_size+6;

  FRAME = malloc(f_size);
  FRAME[0] = FLAG;
  FRAME[1] = address;
  FRAME[2] = control;
  FRAME[3] = FRAME[1] ^ FRAME[2];

  int i;
  for (i = 0; i < p_size; i++)
      FRAME[i+4] = PACKET[i];

  FRAME[i+4] = create_BCC(PACKET, p_size);
  FRAME[i+5] = FLAG;
  return f_size; //tamanho total da trama
}

unsigned char create_BCC(unsigned char * PACKET, int size){
  unsigned char res = 0;
  int i;
  for (i = 0; i < size; i++){
    res ^= PACKET[i];
  }
  
  return res;
}

int state_machine_sup(unsigned char* SET){
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
				else if (ch == C_SET || ch == C_DISC || ch == C_UA || ch == C_RR1 || ch == C_RR0 || ch == C_REJ1 || ch == C_REJ0){
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

	return SET[2];
}
