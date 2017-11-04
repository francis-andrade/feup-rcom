#include "datalink.h"
#include "utils.h"
#include "applicationlayer.h"
#include "alarm.h"

struct termios oldtio, newtio;
s_alarm *alm;
linklayer * ll;

int open_port(const char *destination) {
  ll=malloc(sizeof(linklayer));
  ll->baudrate=BAUDRATE;
  int fd = open(destination, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd < 0) {
    perror(destination);
    exit(-1);
  }

  //saving current port settings
  if (tcgetattr(fd, &oldtio) == -1) {
    perror("tcgetattr");
    exit(-1);
  }

  //configurating newtio and setting input mode
  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = ll->baudrate | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;
  newtio.c_cc[VTIME] = 0;                                           /* inter-character timer unused */
  newtio.c_cc[VMIN] = 1; /* blocking read until 1 chars received */ //old: 5

  //clearing buffer
  tcflush(fd, TCIOFLUSH);

  //applying changes
  if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }
  return fd;
}

int close_port(int fd) {
  // reset TC to old config
  if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }
  // close
  close(fd);
  return 0;
}

int byte_stuff(unsigned char **buf, int size) {
  int i, newsize = size;
  unsigned char *res;

  for (i = 0; i < size; i++) {
    if (((*buf)[i] == FLAG) || ((*buf)[i] == ESCAPE))
      newsize++;
  }

  res = (unsigned char *)malloc(newsize);
  int j = 0;
  for (i = 0; i < size; i++) {
    if ((*buf)[i] == FLAG) {
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

int byte_destuff(unsigned char **buf, int size) {
  unsigned char *res;
  int i, j = 0;
  res = (unsigned char *)malloc(size);
  for (i = 0; i < size; i++, j++) {
    if ((*buf)[i] == ESCAPE) {
      i++;
      if ((*buf)[i] == ESCAPE_E)
        res[j] = ESCAPE;
      else if ((*buf)[i] == FLAG_E)
        res[j] = FLAG;
      else{
        return -1;
	    }
    } else
      res[j] = (*buf)[i];
  }

  res = (unsigned char *)realloc(res, j);
  free(*buf);
  *buf = res;
  return j;
}

int send_frame(unsigned char *frame, int fd) {
  int r = 0;
  if (frame[2] == C_DATA0 || frame[2] == C_DATA1){  //data frame
    int i = 1;
    while (frame[i] != FLAG){
      i++;
    }
    if (write(fd, frame, i + 1) == -1)
      r = -1;
  } else {                                    //supervision frame
    if (write(fd, frame, 5)==-1)
      r = -1;
  }
  return r;
}

int llopen(const char *port, int status) {
  int fd = open_port(port);
  State_Frame sf;

  if (status == SENDER) { //sender
    ll->sequenceNumber = 0;
    unsigned char *frame = malloc(5);
    build_frame_sup(A, C_SET, frame);
    //ALARM
    arm_alarm(TIMEOUT_DURATION, TIMEOUT_TRIES);
    int cnt = 0;
    do {
      if (alm->timeout_flag == 1) {
        alm->timeout_flag = 0;
        send_frame(frame, fd);
      } else if (cnt == 0) {
        send_frame(frame, fd);
      }
      sf = state_machine(fd);
      cnt++;
    } while ((!sf.success || sf.control != C_UA) && alm->count < alm->retries);
    disarm_alarm();
    if (alm->count == alm->retries) {
      printf("Error: Could not open properly\n");
      free(frame);
      return -1;
    }
    free(frame);
    return fd;
  } else { //receiver
    ll->sequenceNumber = 0;
    do {
      sf = state_machine(fd);
    } while (!sf.success || sf.control != C_SET);
    unsigned char *frame = malloc(5);
    build_frame_sup(A, C_UA, frame);
    send_frame(frame, fd);
    free(frame);
    return fd;
  }

  //oops! something went wrong.
  return -1;
}

int llwrite(int fd, unsigned char *buffer, int length) {
 printf("Entering llwrite()\n");
  State_Frame sf;
  unsigned char rr, rej, data;
  if (ll->sequenceNumber == 0) {
    rr = C_RR1;
    rej = C_REJ0;
    data = C_DATA0;
  } else {
    rr = C_RR0;
    rej = C_REJ1;
    data = C_DATA1;
  }

  unsigned char **frame = malloc(sizeof(unsigned char *));
  int size = build_frame_data(A, data, frame, buffer, length);

  //ALARM
  while (1) {
    arm_alarm(TIMEOUT_DURATION, TIMEOUT_TRIES);
    int cnt = 0;
    do {
      if (alm->timeout_flag == 1) {
        alm->timeout_flag = 0;
        send_frame(*frame, fd);
      } else if (cnt == 0) {
        send_frame(*frame, fd);
      }
      sf = state_machine(fd);
      cnt++;
    } while ((!sf.success || !(sf.control == rr || sf.control == rej)) && alm->count < alm->retries);
    disarm_alarm();
    if (alm->count == alm->retries) {
      printf("Error: Could not write properly\n");
      free(frame);
      return -1;
    } else if (sf.control == rr) {
      ll->sequenceNumber = !(ll->sequenceNumber);
      free(frame);
      return size;
    }
  }
}

int llread(int fd, unsigned char *buffer) {
  printf("Entering llread()\n");
  State_Frame sf;
  unsigned char *frame = malloc(5);
  unsigned char ns, rr, rej, comp_ns, comp_rr;

  if (ll->sequenceNumber == 0) {
    ns = C_DATA0;
    rej = C_REJ0;
    rr = C_RR1;
    comp_ns = C_DATA1;
    comp_rr = C_RR0;
  } else {
    ns = C_DATA1;
    rej = C_REJ1;
    rr = C_RR0;
    comp_ns = C_DATA0;
    comp_rr = C_RR1;
  }

  while (1) {
    sf = state_machine(fd);
    if (sf.success == 1 && sf.control == C_SET){
      free(frame);
      return -2;
    } else if (sf.success == 0 && sf.control == ns) {
      printf("Frame had errors, sending REJ\n");
      build_frame_sup(A, rej, frame);
      send_frame(frame, fd);
      return -4;
    } else if (sf.success == 1 && sf.control == ns) {
      ll->sequenceNumber = !(ll->sequenceNumber);
      unsigned int i;
      for (i = 0; i < sf.size; i++) {
        buffer[i] = sf.data[i];
      }
      build_frame_sup(A, rr, frame);
      send_frame(frame, fd);
      break;
    } else if(sf.success==1 && sf.control == C_DISC){
	      free(frame);
	      return -3;
    }
    else if(sf.control==comp_ns){
      build_frame_sup(A, comp_rr, frame);
      send_frame(frame, fd);
      return -5;
    }
  }

  free(frame);
  return sf.size;
}

int llclose(int fd, int status){
  unsigned char *frame = malloc(5);
  State_Frame sf;
  build_frame_sup(A, C_DISC, frame);

  if (status == SENDER) {
    //ALARM
    arm_alarm(TIMEOUT_DURATION, TIMEOUT_TRIES);
    int cnt = 0;
    do {
      if (alm->timeout_flag == 1) {
        alm->timeout_flag = 0;
        send_frame(frame, fd);
      } else if (cnt == 0) {
        send_frame(frame, fd);
      }
      sf = state_machine(fd);
      cnt++;
    } while ((!sf.success || sf.control != C_DISC) && alm->count < alm->retries);
    disarm_alarm();
    if (alm->count == alm->retries) {
      printf("Error: Could not close properly\n");
      return -1;
    }

    build_frame_sup(A, C_UA, frame);
    send_frame(frame, fd);
  } else {
    //ALARM
    arm_alarm(TIMEOUT_DURATION, TIMEOUT_TRIES);
    int cnt = 0;
    do {
      if (alm->timeout_flag == 1) {
        alm->timeout_flag = 0;
        send_frame(frame, fd);
      } else if (cnt == 0) {
        send_frame(frame, fd);
      }
      sf = state_machine(fd);
      cnt++;
    } while ((!sf.success || sf.control != C_UA) && alm->count < alm->retries);
    disarm_alarm();
    if (alm->count == alm->retries) {
      printf("Error: Could not close properly\n");
      free(frame);
      return -1;
    }
  }
  sleep(1);
  tcsetattr(fd, TCSANOW, &oldtio);
  free(frame);
  return close_port(fd);
}
