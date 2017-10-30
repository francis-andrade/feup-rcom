#include "utils.h"
#include "alarm.h"
#include "datalink.h"

s_alarm *alm;

int build_frame_sup(unsigned char address, unsigned char control, unsigned char *FRAME) {
  //printf("Entered build_frame_sup() with address=0x%x and control=0x%x\n",address,control);  
  FRAME[0] = FLAG;
  FRAME[1] = address;
  FRAME[2] = control;
  FRAME[3] = FRAME[1] ^ FRAME[2];
  FRAME[4] = FLAG;
  return 5;
}

int build_frame_data(unsigned char address, unsigned char control, unsigned char **FRAME, unsigned char *PACKET, int length) {
  //printf("Entered build_frame_data() with address=%c, control=%c and length=%d\n",address,control,length);
  unsigned char *frame_to_stuff = malloc(length + 1);
  int i;
  for (i = 0; i < length; i++) {
    frame_to_stuff[i] = PACKET[i];
  }

  frame_to_stuff[i] = create_BCC(PACKET, length);
  int new_size = byte_stuff(&frame_to_stuff, length + 1);
  //printf("build_frame_data(): Reallocating frame memory after byte stuffing...\n");
  *FRAME = malloc(new_size + 5);
  //printf("build_frame_data(): Reallocated *FRAME\n");
  (*FRAME)[0] = FLAG;
  (*FRAME)[1] = address;
  (*FRAME)[2] = control;
  (*FRAME)[3] = (*FRAME)[1] ^ (*FRAME)[2];
  //printf("build_frame_data(): Defined the first 4 elements of *FRAME\n");
  for (i = 0; i < new_size; i++) {
    (*FRAME)[i + 4] = frame_to_stuff[i];
  }
  (*FRAME)[i + 5] = FLAG;
  //printf("Freeing frame_to_stuff memory...\n");
  free(frame_to_stuff);
  //printf("Exiting build_frame_data() with newsize=%d\n",new_size);  
  return new_size + 5; //tamanho total da trama
}

unsigned char create_BCC(unsigned char *PACKET, int size) {
  //printf("Entered create_BCC() with size=%d\n",size);  
  unsigned char res = 0;
  int i;
  for (i = 0; i < size; i++) {
    res ^= PACKET[i];
  }

  return res;
}

//I THINK THERE IS A BUG WITH sf.success
State_Frame state_machine(int fd) {
  printf("Entered state_machine()\n");

  unsigned char ch;
  unsigned char * datatmp = (unsigned char *) malloc(255);
  State state = S_START;
  int i = 0;
  State_Frame sf;
  sf.success = 1;
  int curr_count = alm->count;
  //printf("Entering state machine cycle\n");
  //run until we reach the frame's end or until being timed-out
  while (state != S_END && curr_count == alm->count) {
    if (read(fd, &ch, 1) <= 0)
      continue;

    //printf("State=%d\n",state);
    switch (state) {
    case S_START:
      if (ch == FLAG) {
        state = S_FLAG;
      } else
        state = S_START;
      break;

    case S_FLAG:
      if (ch == FLAG)
        state = S_FLAG;
      else if (ch == A) {
        sf.address = ch;
        state = S_ADDRESS;
      } else
        state = S_START;
      break;

    case S_ADDRESS:
      if (ch == FLAG)
        state = S_FLAG;
      else if (ch == C_SET || ch == C_DISC || ch == C_UA || ch == C_RR1 ||
               ch == C_RR0 || ch == C_REJ1 || ch == C_REJ0 || ch == C_DATA0 || ch == C_DATA1) {
        state = S_CONTROL;
        sf.control = ch;
      } else
        state = S_START;
      break;

    case S_CONTROL:
      if (ch == FLAG)
        state = S_FLAG;
      else if (ch == (sf.address ^ sf.control)) {
        if (sf.control == C_DATA0 || sf.control == C_DATA1) {
          state = S_DN;
        } else
          state = S_BCC1;
      } else
        state = S_START; //ignorar trama
      break;

    case S_BCC1:
      if (ch == FLAG) {
        state = S_END;
        printf("Supervision frame received.\n");
      } else
        state = S_START;
      break;

    case S_DN:
      datatmp[i] = ch;
      if (ch == FLAG) {
        printf("Data frame received. Destuffing frame data...\n");        
        int size = byte_destuff(&datatmp, i);
        if (datatmp[i - 1] == create_BCC(datatmp, i-1)) {
          sf.size = size-1;
          sf.data = datatmp;
          state = S_END;
        } else {
          printf("Error with BCC2, exiting state machine\n");
          sf.success = 0;
          return sf;
        }
      } else
        i++;
      break;

    default:
      return sf;
    }
  }

  printf("Exiting state machine...\n");
  return sf;
}
