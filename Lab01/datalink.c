#include "datalink.h"
#include "utils.h"
#include "applicationlayer.h"
#include "alarm.h"

struct termios oldtio, newtio;
s_alarm * alm;
linklayer * ll;

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

int byte_stuff(unsigned char ** buf, int size){
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
    j++;
  }

  free(*buf);
  *buf = res;
  return newsize;
}

int byte_destuff(unsigned char** buf, int size){
  //TODO assegurar que também podemos incluir o bcc aqui
  unsigned char * res;
  int i, newsize = size;
  int j = 0;
  res = (unsigned char *)malloc(newsize);
  for (i = 0; i < size; i++){
    if((*buf)[i] == ESCAPE){
      i++;
      if ((*buf)[i] == ESCAPE_E)
        res[j] = ESCAPE;
      else if ((*buf)[i] == FLAG_E)
        res[j] = FLAG;
      else return -1;
    }
    else
	res[j]=(*buf)[i];
    j++;
  }

  res = (unsigned char *) realloc(res, j);
  free(*buf);
  *buf=res;
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
  State_Frame sf;

  if (status == SENDER){ //sender
    unsigned char * frame;
    build_frame_sup(A, C_SET, frame);
    //ALARM
      init_alarm();
      arm_alarm(3,3);
      do{
	//tcflush(fd, TCIOFLUSH);//CHECK IF THIS IS CORRECT
	if(alm->timeout_flag==1){
		alm->timeout_flag=0;
		send_frame(frame,fd);	
	}
        sf = state_machine(fd);
      } while ((!sf.success || sf.control != C_UA) && alm->count<alm->retries);//I THINK THERE IS A BUG WITH sf.success
      disarm_alarm();
      if(alm->count==alm->retries){
	printf("Error: Could not open properly\n");
	return -1;
      }
      return fd;
     
  } 
  else { //receiver
    do{
        sf = state_machine(fd);
      } while (!sf.success || sf.control != C_SET);//I THINK THERE IS A BUG WITH sf.success
    unsigned char * frame;
    build_frame_sup(A, C_UA, frame);
    send_frame(frame, fd);
    return fd;
    
  }

  //oops! something went wrong.
  return -1;
}

int llwrite(int fd, unsigned char* buffer, int length){
      State_Frame sf;
      unsigned char rr, rej, data;
      if(ll->sequenceNumber==0){
	rr=C_RR1;
	rej=C_REJ0;
	data=C_DATA0;
      }
      else{
	rr=C_RR0;
	rej=C_REJ1;
	data=C_DATA1;
      }
      unsigned char * frame;
      int size=build_frame_data(A, data, frame, buffer, length);
  
      //ALARM
      init_alarm();
      int send=1;
      while(send){
      arm_alarm(3,3);
      do{
	//tcflush(fd, TCIOFLUSH);//CHECK IF THIS IS CORRECT
	if(alm->timeout_flag==1){
		alm->timeout_flag=0;
		send_frame(frame, fd);
	}
        sf = state_machine(fd);
      } while ((!sf.success || sf.control != rr || sf.control != rej ) && alm->count<alm->retries);//I THINK THERE IS A BUG WITH sf.success
      disarm_alarm();
      if(alm->count==alm->retries){
	printf("Error: Could not open properly\n");
	return -1;
      }
      else if(sf.control==rr){
	ll->sequenceNumber=(ll->sequenceNumber+1)%2;
	return size;
     }
     else{
		send=1;	
	}
     }

  
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
    sf = state_machine(fd);
  }

  byte_destuff(frame, sf.size);



//TODO ler trama
//TODO enviar RR/REJ
}

int llclose(int fd, int status){
    unsigned char* frame;
    State_Frame sf;
    build_frame_sup(A, C_DISC, frame);

    if (status == SENDER) {

      //ALARM
      init_alarm();
      arm_alarm(3,3);
      do{
	//tcflush(fd, TCIOFLUSH);//CHECK IF THIS IS CORRECT
	if(alm->timeout_flag==1){
		alm->timeout_flag=0;
		send_frame(frame,fd);	
	}
        sf = state_machine(fd);
      } while ((!sf.success || sf.control != C_DISC) && alm->count<alm->retries);//I THINK THERE IS A BUG WITH sf.success
      disarm_alarm();
      if(alm->count==alm->retries){
	printf("Error: Could not close properly\n");
	return -1;
      }
   
      build_frame_sup(A, C_UA, frame);
      send_frame(frame, fd);
    } else {
      //ALARM
      init_alarm();
      arm_alarm(3,3);
      do{
	if(alm->timeout_flag==1){
		alm->timeout_flag=0;
		send_frame(frame,fd);	
	}
        sf = state_machine(fd);
      } while ((!sf.success || sf.control != C_UA) && alm->count<alm->retries);//I THINK THERE IS A BUG WITH sf.success
      disarm_alarm();
      if(alm->count==alm->retries){
	printf("Error: Could not close properly\n");
	return -1;
      }

    }
    tcsetattr(fd,TCSANOW,&oldtio); 
    return close_port(fd);
}

