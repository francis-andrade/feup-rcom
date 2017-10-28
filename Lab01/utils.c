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

State_Frame state_machine(int fd){
  unsigned char ch, datatmp[255];
	State state = S_START;
  int i = 0;
  State_Frame sf;
  sf.success = 1;

  //run until we reach the frame's end or until being timed-out
	while (state != S_END && !timeout_flag){
		if (read(fd, &ch, 1)<=0)
      continue;

		switch(state){
      case S_START:
				if(ch == FLAG){
					state = S_FLAG;}
				else state = S_START;
			break;

			case S_FLAG:
				if (ch == FLAG) state = S_FLAG;
				else if (ch == A){
          sf.address = ch;
					state = S_ADDRESS;}
				else state = S_START;
      break;

      case S_ADDRESS:
				if (ch == FLAG) state = S_FLAG;
				else if (ch == C_SET || ch == C_DISC || ch == C_UA || ch == C_RR1 ||
            ch == C_RR0 || ch == C_REJ1 || ch == C_REJ0 || ch == C_DATA0 || ch == C_DATA1){
					state = S_CONTROL;
					sf.control = ch;}
        else state = S_START;
			break;

      case S_CONTROL:
				if (ch == FLAG) state = S_FLAG;
				else if (ch == (sf.address^sf.control)){
          if (sf.control == C_DATA0 || sf.control == C_DATA1){
              state = S_DN;
          } else state = S_BCC1;
        } else state = S_START; //ignorar trama
      break;

			case S_BCC1:
				if (ch == FLAG){
					state = S_END;}
				else state = S_START;
			break;

      case S_DN:
        datatmp[i] = ch;
        if (ch == FLAG){
          if (datatmp[i-1] == create_BCC(datatmp, i-2)){
            sf.size = i-2;
            sf.data = (unsigned char*) malloc(sf.size);
            state = S_END;
          } else {
            sf.success = 0;
            return sf;
          }
        } else i++;
      break;

      default:
      return sf;

		}
	}

	return sf;
}
