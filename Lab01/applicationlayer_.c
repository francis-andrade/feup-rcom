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


int sender(const char* port, const char* filename){
  int i, res=0, packet_size=0;
  size_t filesize=0;

  printf("NSerial: Sender\n");

  //init app
  ApplicationLayer app;
  app.mode = SENDER;
  app.port = (char*)port;
  app.filename = (char*)filename;

  // open serial bus fd
  app.fd = llopen(app.port, app.mode);
  if (app.fd>0)
    printf("Successfully opened port.\n");
  else {
    printf("Failed to open port. Returned code %d\n",app.fd);
    return -1;
  }

  //open file (e.g. penguin)
  FILE *fp = fopen(app.filename, "rb");
  if (fp>0)
    printf("Successfully opened file %s.\n",app.filename);
  else {
    printf("Failed to open file %s.\n",app.filename);
    return -1;
  }

  // get the filesize
  fseek(fp, 0, SEEK_END);
  filesize = ftell(fp);
  rewind(fp);

  // init chunk, packet arrays
  unsigned char chunk[CHUNK_SIZE];
  unsigned char packet[CHUNK_SIZE+4];

  // send START packet (timeouts are accounted for within llwrite, using timeout_flag)
  packet_size = create_control_packet(packet, app.filename, AL_C_START, filesize);
  printf("Sending Start-Packet...");
  res = llwrite(app.fd, packet, packet_size);
  if (res<=0){
    printf("\t[Start-Packet]: llwrite() returned %d. Exiting...\n",res);
    return -1;
  }
  else 
    printf("\t[Start-Packet]: Successfully sent.\n");

  // send all other packets
  printf("Sending Data-Packets...");
  for (i=0; fread(chunk, CHUNK_SIZE, 1, fp)>0 ; ++i){ // read a chunk of the file
    packet_size = create_data_packet(i%256, chunk, CHUNK_SIZE, packet); // i = applayer seqN
    res = llwrite(app.fd, packet, packet_size);
    if (res<=0){
      printf("\t[Data-Packet %d]: llwrite() returned %d. Exiting...\n",i,res);
      return -1;
    }
    else 
      printf("\t[Data-Packet %d]: Successfully sent.\n",i);
  }
  printf("\tSuccessfully sent all Data-Packets\n");
  
  //send END packet
  packet_size = create_control_packet(packet, app.filename, AL_C_END, filesize);
  printf("Sending End-Packet...");
  res = llwrite(app.fd, packet, packet_size);
  if (res<=0){
    printf("\t[End-Packet]: llwrite() returned %d. Exiting...\n",res);
    return -1;
  }
  else 
    printf("\t[End-Packet]: Successfully sent.\n");

  //end runtime
  printf("Attempting to disconnect...\n");
  res = llclose(app.fd, app.mode);
  if (res==0) 
    printf("\tSuccessfully disconnected.");
  else 
    printf("\tFailed to disconnect.");
  fclose(fp);
  return 0;
}


int receiver(const char* port){
  int i, j, res;
  unsigned char buffer[256];  //TODO this 256 should be defined in header
  unsigned char * packet_start=0, * packet_end=0;
  int packet_start_size=0, packet_end_size=0, packet_sn=0;
  FILE *fp = 0;

  printf("NSerial: Receiver\n");

  //init app
  ApplicationLayer app;
  app.mode = RECEIVER;
  app.port = (char*)port;

  int state = 0;
  while (state!=3) switch (state){
    // llopen - le init
    case 0:
      //open serial bus
      app.fd = llopen(app.port, app.mode);
      if (app.fd>0)
        printf("Successfully opened port %s\n",app.port);
      else {
        printf("Failed to open port %s\n",app.port);
        break;
      }
        
      //open temporary file
      if (fp!=0) fclose(fp);
      fp = fopen(TMP_FILENAME, "wb");
      if (fp>0){
        printf("Opened temporary file '%s'\n",TMP_FILENAME);
        state=1;
      }
      else {
        printf("Failed to open temporary file '%s'\n",TMP_FILENAME);
      }
    break;

    // llread
    case 1:
      res = llread(app.fd, buffer);
      if (res>0){
        // is it a start packet?
        if (buffer[0]==AL_C_START){
          printf("Reading Start-Packet...\n");
          //copy to packet_start
          free(packet_start);
          packet_start_size = res;
          packet_start = (unsigned char *) malloc(packet_start_size);
          memcpy(packet_start, buffer, packet_start_size);
          //get filename, filesize, others
          for (i=1; i<res; ++i){
            unsigned char type = buffer[i++];
            unsigned char length = buffer[i++];
            //TLV = filename
            if (type==AL_T_NAME){
              printf("\t[Start-Packet]: Processing filename...\n");
              app.filename = (char*) malloc(length);
              for (j=0; j<length; ++j)
                app.filename[j] = buffer[i++];
              printf("\t[Start-Packet]: Filename = %s\n",app.filename);
            }
            //TLV = filesize
            else if (type==AL_T_SIZE){
              printf("\t[Start-Packet]: Processing filesize...\n");
              app.filesize = 0;
              for (j=0; j<length; ++j)
                app.filesize += app.filesize*256 + buffer[i++];
              printf("\t[Start-Packet]: Filename = %s\n",app.filesize);
            }
            //TLV = ???
            else {
              printf("\tError: packet seems to be of an unknown type..\n");
              return -1;
            }
          }
          printf("\t[Start-Packet]: Successfully received.\n");
        }
        // is it a data packet?
        else if (buffer[0]==AL_C_DATA){
          printf("Reading Data-Packet[%d]...\n",packet_sn);
          if (res<5)
            printf("\tError: Data-Packet has a length of less than 5. (%d)\n",res);
          else {
            // packet sequence number
            if (buffer[1]!=packet_sn)
              printf("\tWarning: Expected packet sequence number %d. Received %d\n",packet_sn, buffer[1]);
            else {
              size_t length = ((size_t)buffer[2])*256 + ((size_t)buffer[3]);
              if (length != res-4)
                printf("\tWarning: Packet-body bytes read (%d) differ from number expected (res-4, res=%d)\n",(int)length,res);
              fwrite(buffer+4, res-4, 1, fp);
              ++packet_sn;
            }
          }
        }
        // is it an end packet?
        else if (buffer[0]==AL_C_END){
          printf("Reading End-Packet...\n");
          //copy to packet_end
          free(packet_end);
          packet_end_size = res;
          packet_end = (unsigned char *) malloc(packet_end_size);
          memcpy(packet_end, buffer, packet_end_size);
          //check whether packet_end == packet_start
          if (memcmp(packet_start+1, packet_end+1, packet_end_size-1)!=0) //skip control byte
            printf("\tWarning: Start-Packet differs from End-Packet\n");
          //copy current file to another file
          fclose(fp);
          if (!rename(TMP_FILENAME, app.filename))
            printf("\tSuccessfully transfered %s.\n",app.filename);
          else 
            printf("\tError: Failed to change filename of temporary file.\n");
          free(app.filename);
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
      printf("Attempting to disconnect...\n");
      res = llclose(app.fd, app.mode);
      if (res==0) 
        printf("\tSuccessfully disconnected.");
      else 
        printf("\tFailed to disconnect.");
      if (fp!=0)
        fclose(fp);     
      state = 3;
    break;
  };

  //end runtime
  return 0;
}