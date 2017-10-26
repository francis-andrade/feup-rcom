#include "datalink.h"

struct termios oldtio, newtio;

int open_port(const char* destination){
    int fd = open(destination, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd <0) { perror(destination); exit(-1); }

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

	return fd;
}

int close_port(int fd){
    // reset TC to old config
    if (tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);//TODO averiguar se podemos ter o -1 aqui
    }
    // close
    close(fd);
    return 0;
}

void byte_stuff(unsigned char** buf, int size){
  //TODO assegurar que também podemos incluir o bcc aqui
  int i, newsize = size;
  unsigned char *res;

  for (i = 0; i < size; i++){
    if (((*buf)[i] == FLAG) || ((*buf)[i] == ESCAPE))
      newsize++;
  }

  res = (unsigned char *)malloc(newsize);
  int j = 0;
  for (i = 0; i < size; i++){
    if ((*buf)[i] == FLAG){
      res[j] = ESCAPE;
      j++;
      res[j] = FLAG_E;
    } else if ((*buf)[i] == ESCAPE) {
      res[j] = ESCAPE;
      j++;
      res[j] = ESCAPE_E;
    } else
      res[j] = (*buf)[i];
    ++j;
  }

  free(*buf);
  *buf = res;
}

void byte_destuff(unsigned char** buf, int size){
  //TODO assegurar que também podemos incluir o bcc aqui
  unsigned char * res;
  int i, newsize = size;
  int j = 0;

  for (i = 0; i < size; i++){
    if((*buf)[i] == ESCAPE){
      i++;
      if ((*buf)[i] == ESCAPE_E)
        (*buf)[j] = ESCAPE;
      else if ((*buf)[i] == FLAG_E)
        (*buf)[j] = FLAG;
      else return -1;
    }
    j++;
  }

  *buf = (unsigned char *) realloc(*buf, j);
  return j;
}

int send_frame(unsigned char* frame, int fd){
  if(frame[2] == C_DATA0 || frame[2] == C_DATA1){ //data frame
    int i = 1;
    while(frame[i]!=FLAG)
      i++;
    write(fd, frame, i+1);
  } else {                                        //supervision frame
    write(fd, frame, 5);
  }
}

int llopen(const char* port, int status){
  int fd = open_port(port);
  unsigned char* frame;
  if (status == RECEIVER){ //receiver
    State_Frame sf;
    do sf = state_machine(frame, fd);
      while (!sf.success || sf.control != C_SET);
    build_frame_sup(A, C_UA, frame);
    send_frame(frame, fd);
  } else {                    //sender
    build_frame_sup(A, C_SET, frame);
    send_frame(frame, fd);

    //TODO:
    //criar alarme
    //maq estados
    //repetir caso preciso
  }

}

int llclose(int fd, int status){
    unsigned char* frame;
    State_Frame sf;
    build_frame_sup(A, C_DISC, frame);

    if (status == SENDER) {
      send_frame(frame, fd);

      //TODO implementar alarme e repeticoes aqui também

      do{
        sf = state_machine(frame, fd);
      } while (!sf.success || sf.control != C_DISC);

      build_frame_sup(A, C_UA, frame);
      send_frame(frame, fd);
    } else {
      send_frame(frame, fd);

      //TODO alarme

      do{
        sf = state_machine(frame, fd);
      } while (!sf.success || sf.control != C_UA);
    }
    tcsetattr(fd,TCSANOW,&oldtio); 
    return close_port(fd);
}

int llwrite(int fd, unsigned char* buffer, int length){
  unsigned char frame[MAX_SIZE];
  
//TODO obter trama
//TODO enviar trama
//TODO alarme+timeout
//TODO esperar pelo RR/REJ
//TODO caso REJ, reenviar de acordo com o pedido
}

int llread(int fd, unsigned char* buffer){
  State_Frame sf;
  unsigned char frame[MAX_SIZE];
  //TODO se receber um set aquando da primeira execucao, mandar ua para reestabelecer execucao
  //TODO fazer byte unstuffing e mandar para o buffer

  while(1){
    sf = state_machine(frame, fd);
  }

  byte_destuff(frame, sf.size);



//TODO ler trama
//TODO enviar RR/REJ
}
