//  Frames
#define FLAG 0x7e
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07
#define C_DISC 0x0B
#define C_RR0 0x05
#define C_REJ0 0x01
#define C_RR1 0x85
#define C_REJ1 0x81

//  Packets
#define PC_CONT 0x01
#define PC_START 0x02
#define PC_END 0x03

int state_machine_sup();
int state_machine_data();
int build_frame_sup(unsigned char address, unsigned char control, unsigned char * FRAME);
int build_frame_data(unsigned char address, unsigned char control, unsigned char * FRAME, unsigned char * PACKET);
unsigned char create_BCC(unsigned char * PACKET, int size);
