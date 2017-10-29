#include "applicationlayer.h"
#include "datalink.h"


int create_data_packet(int sequence_no, unsigned char *chunk, size_t chunk_size, unsigned char *packet){
  // packet header
  packet[0] = AL_C_DATA;         // control = 1
  packet[1] = sequence_no % 256; // sequencia %255
  packet[2] = chunk_size / 256;  // l2
  packet[3] = chunk_size % 256;  // l1

  // packet body
  int i, j = 4;
  for (i = 0; i < chunk_size; ++i, ++j)
    packet[j] = chunk[i];

  return j;
}


int create_control_packet(unsigned char * packet, const char* filename, const unsigned char control, size_t filesize){
  int i = 0, j;
  packet[i++] = control;

  //1st TLV - file size
  packet[i++] = AL_T_SIZE;
  packet[i++] = sizeof(size_t);
  size_t temp = filesize;

  for (j=0; j<sizeof(size_t); ++j, ++i){
    packet[i] = temp/256;
    temp = temp%256;
  }

  //2nd TLV - filename
  packet[i++] = AL_T_NAME;
  packet[i++] = strlen(filename);

  for (j=0; j<strlen(filename); ++j, ++i){
    packet[i] = filename[j];
  }

  return i;
}


int sender(ApplicationLayer app){
  int res, packet_size, i;
  size_t filesize;

  // open serial bus fd
  app.fd = llopen(app.port, SENDER);

  //open file (e.g. penguin)
  FILE *fp = fopen(app.filename, "rb");

  // get the filesize
  fseek(fp, 0, SEEK_END);
  filesize = ftell(fp);
  rewind(fp);

  // init chunk, packet arrays
  unsigned char chunk[CHUNK_SIZE];
  unsigned char packet[CHUNK_SIZE+4];

  // send START packet (timeouts are accounted for within llwrite, using timeout_flag)
  packet_size = create_control_packet(packet, app.filename, AL_C_START, filesize);
  do {
    res = llwrite(app.fd, packet, packet_size);
  } while (res<0);

  // send all other packets
  for (i=0; fread(chunk, CHUNK_SIZE, 1, fp)>0 ; ++i){ // read a chunk of the file
    packet_size = create_data_packet(i%256, chunk, CHUNK_SIZE, packet); // i = applayer seqN
    //loop
    do {  
      res = llwrite(app.fd, packet, packet_size);
    } while (res<0);
  }
  
  //send END packet
  packet_size = create_control_packet(packet, app.filename, AL_C_END, filesize);
  do {
    res = llwrite(app.fd, packet, packet_size);
  } while (res<0);

  //end runtime
  llclose(app.fd, SENDER);
  fclose(fp);
  return 0;
}


int receiver(ApplicationLayer app){
  int i, j, res;
  unsigned char buffer[256];  //TODO this 256 should be defined in header
  unsigned char * packet_start=0, * packet_end=0;
  int packet_start_size=0, packet_end_size=0, packet_sn=0;
  FILE *fp;

  int state = 0;
  while (state!=3) switch (state){
    // llopen - le init
    case 0:
      //open serial bus
      app.fd = llopen(app.port, SENDER);
      if (app.fd<=0)
        
      //open temporary file
      if (fp!=0) 
        fclose(fp);
      fp = fopen(TMP_FILENAME, "wb");
    break;

    // llread
    case 1:
      res = llread(app.fd, buffer);
      if (res>0){
        // is it a start packet?
        if (buffer[0]==AL_C_START){
          //copy to packet_start
          free(packet_start);
          packet_start_size = res;
          packet_start = (unsigned char *) malloc(packet_start_size);
          memcpy(packet_start, buffer, packet_start_size);
          //get filename, filesize, others
          for (i=1; i<res; ++i){
            unsigned char type = buffer[i+1];
            unsigned char length = buffer[i+2];
            //TLV = filename
            if (type==AL_T_NAME){
              app.filename = (char*) malloc(length);
              for (j=0; j<length; ++j)
                app.filename[j] = buffer[i+j];
            }
            //TLV = filesize
            else if (type==AL_T_SIZE){
              app.filesize = 0;
              for (j=0; j<length; ++j)
                app.filesize += app.filesize*256 + buffer[i+j];
            }
            //TLV = ???
            else {
              printf("Error: packet seems to be of an unknown type..\n");
              return -1;
            }
          }
        }
        // is it a data packet?
        else if (buffer[0]==AL_C_DATA){
          if (res<5)
            printf("Error: Data-Packet has a length of less than 5. (%d)\n",res);
          else {
            // packet sequence number
            if (buffer[1]!=packet_sn)
              printf("Warning: Expected packet sequence number %d. Received %d\n",packet_sn, buffer[1]);
            else {
              size_t length = ((size_t)buffer[2])*256 + ((size_t)buffer[3]);
              if (length != res-4)
                printf("Warning: Packet-body bytes read (%d) differ from number expected (res-4, res=%d)\n",(int)length,res);
              fwrite(buffer+4, res-4, 1, fp);
            }
          }
        }
        // is it an end packet?
        else if (buffer[0]==AL_C_END){
          //copy to packet_end
          free(packet_end);
          packet_end_size = res;
          packet_end = (unsigned char *) malloc(packet_end_size);
          memcpy(packet_end, buffer, packet_end_size);
          //check whether packet_end == packet_start
          if (memcmp(packet_start+1, packet_end+1, packet_end_size-1)!=0) //skip control byte
            printf("Warning: Start-Packet differs from End-Packet\n");
          //copy current file to another file
          fclose(fp);
          if (!rename(TMP_FILENAME, app.filename))
            printf("Error: Failed to change filename of temporary file.\n");
          state=2;
        }
      }
      else if (res==-2){
        state=0;
        printf("llread() retuned: -2 (we have received a SET, so back to llopen())\n");
      }
      else if (res==-3){
        state=2;
      }
      else {
        printf("Error: llread() retuned: %d\n",res);
        return -2;
      }
    break;

    case 2: 
      llclose(app.fd, RECEIVER);
      if (fp!=0)
        fclose(fp);     
      state = 3;
    break;
  };

  //end runtime
  return 0;
}