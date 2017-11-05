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

void print_packet(unsigned char * packet, size_t packet_size){
  int i;
  for (i=0; i<packet_size; ++i)
    printf("\t\tpacket[%d] = 0x%x\n",i,packet[i]);
}

int create_control_packet(unsigned char * packet, const char* filename, const unsigned char control, size_t filesize){
  int i = 0, j;
  packet[i++] = control;

  //1st TLV - file size
  packet[i++] = AL_T_SIZE;
  packet[i++] = sizeof(size_t);
  size_t temp = filesize;
  for (j=sizeof(size_t)-1; j>=0; --j){
    packet[i+j] = (unsigned char)(temp%256);
    temp = temp/256;
  }
  i+=sizeof(size_t);

  //2nd TLV - filename
  packet[i++] = AL_T_NAME;
  packet[i++] = strlen(filename)+1;
  for (j=0; j<strlen(filename)+1; ++j, ++i){
    packet[i] = (unsigned char)(filename[j]);
  }

  return i;
}

int sender(const char* port, const char* filename){
  int i, res=0, packet_size=0;

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
  app.filesize = ftell(fp);
  rewind(fp);
  printf("File '%s' is of size %ld bytes\n",app.filename, app.filesize);

  // init chunk, packet arrays
  int CHUNK_SIZE = stats->chunk_size;
  unsigned char chunk[CHUNK_SIZE];
  unsigned char packet[CHUNK_SIZE+4];

  // send START packet (timeouts are accounted for within llwrite, using timeout_flag)
  packet_size = create_control_packet(packet, app.filename, AL_C_START, app.filesize);
  printf("Sending Start-Packet...\n");
  res = llwrite(app.fd, packet, packet_size);
  if (res<=0){
    printf("\t[Start-Packet]: llwrite() returned %d. Exiting...\n",res);
    return -1;
  }
  else
    printf("\t[Start-Packet]: Successfully sent.\n");

  // send all other packets
  printf("Sending Data-Packets...");
  size_t bytes_left = app.filesize;
  for (i=0; bytes_left>0; ++i){ // read a chunk of the file
    //calc bytes chunk
    size_t bytes_chunk;
    if (bytes_left>CHUNK_SIZE)
      bytes_chunk = CHUNK_SIZE;
    else
      bytes_chunk = bytes_left;
    //copy bytes from file
    if (fread(chunk, bytes_chunk, 1, fp)<=0)
      printf("\t[Data-Packet %d]: failed to read chunk from file\n",i);
    //create packet
    packet_size = create_data_packet(i%256, chunk, bytes_chunk, packet); // i = applayer seqN
    //send packet
    res = llwrite(app.fd, packet, packet_size);
    if (res<=0){
      printf("\t[Data-Packet %d]: llwrite() returned %d. Exiting...\n",i,res);
      return -1;
    }
    printf("\t[Data-Packet %d]: Successfully sent.\n",i);
    bytes_left -= bytes_chunk;
  }
  printf("\tSuccessfully sent all Data-Packets\n");

  //send END packet
  packet_size = create_control_packet(packet, app.filename, AL_C_END, app.filesize);
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
    printf("\tSuccessfully disconnected.\n");
  else
    printf("\tFailed to disconnect.\n");
  fclose(fp);
  return 0;
}

int receiver(const char* port){
  int i, j, res;
  unsigned char buffer[256];
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
    //initial state (llopen)
    case 0:
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
        packet_sn = 0;
        //stats R start
        stats->R = 0;
        //stats R end
      } else {
        printf("Failed to open temporary file '%s'\n",TMP_FILENAME);
      }
    break;
    
    //llread
    case 1:
      res = llread(app.fd, buffer);
      // res>0 -> success!
      if (res>0){
        //stats R start
        stats->R += res;
        //stats R end
        //analyse the control byte
        switch(buffer[0]){
        // is it a start packet?
          case AL_C_START:
            printf("Reading Start-Packet...\n");
            //copy to packet_start
            free(packet_start);
            packet_start_size = res;
            packet_start = (unsigned char *) malloc(packet_start_size);
            memcpy(packet_start, buffer, packet_start_size);
            //get filename, filesize, others
            for (i=1; i<res; ){
              printf("\t[Start-Packet]: Processing new TLV... (%d bytes left, %x)\n",res-i,buffer[i]);
              unsigned char type = buffer[i++];
              unsigned char length = buffer[i++];
              switch (type){
                //TLV = filename
                case AL_T_NAME:
                  printf("\t\t[Start-Packet]: Processing filename...\n");
                  app.filename = (char*) malloc(length);
                  for (j=0; j<length; ++j)
                    app.filename[j] = (char)(buffer[i++]);
                  printf("\t\t[Start-Packet]: Filename = %s\n",app.filename);
                break;

                //TLV = filesize
                case AL_T_SIZE:
                  printf("\t\t[Start-Packet]: Processing filesize...\n");
                  app.filesize = 0;
                  for (j=0; j<length; ++j)
                    app.filesize = app.filesize*256 + (size_t)(buffer[i++]);
                  printf("\t\t[Start-Packet]: Filesize = %ld\n",app.filesize);
                break;

                //TLV = ???
                default:
                  printf("\t\tError: packet seems to be of an unknown type..\n");
                  return -1;
                break;
              }
            }
            printf("\t[Start-Packet]: Successfully received.\n");
          break;
          // is it a data packet?
          case AL_C_DATA:
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
          break;
          // is it an end packet?
          case AL_C_END:
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
          break;
        }
      }
      // res==-2 -> received a SET
      else if (res==-2){
        state=0;
        printf("llread() retuned: -2 (we have received a SET, so back to llopen())\n");
      }
      // res==-3 -> received a DISC
      else if (res==-3){
        state=2;
      }
      // res==-4 -> retry
      else if (res==-4  || res==-5){
        break;
      }
      // res==??? -> undefined behaviour
      else {
        printf("Error: llread() retuned: %d\n",res);
        return -2;
      }
    break;

    // llclose()
    case 2:
      printf("Attempting to disconnect...\n");
      res = llclose(app.fd, app.mode);
      if (res==0)
        printf("\tSuccessfully disconnected.\n");
      else
        printf("\tFailed to disconnect.\n");
      if (fp!=0)
        fclose(fp);
      state = 3;
    break;
  };

  //end runtime
  return 0;
}
