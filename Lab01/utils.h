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

typedef enum{
  S_START, S_FLAG, S_ADDRESS, S_CONTROL, S_BCC1, S_END, S_DN, S_BCC2
} State;

typedef struct{
  int success;     // literalmente um booleano
  unsigned char address;
  unsigned char control;
  unsigned char* data;
  int size;
} State_Frame;

int state_machine();
int build_frame_sup(unsigned char address, unsigned char control, unsigned char * FRAME);
int build_frame_data(unsigned char address, unsigned char control, unsigned char * FRAME, unsigned char * PACKET);
unsigned char create_BCC(unsigned char * PACKET, int size);
