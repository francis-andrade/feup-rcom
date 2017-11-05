#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include "applicationlayer.h"
#include "datalink.h"
#include "alarm.h"

s_stats * stats;

void print_usage(){
  printf("Usage:\tnserial SerialPort receiver OR nserial SerialPort sender filename \n\tex: nserial /dev/ttyS1 receiver \n\tex: nserial /dev/ttyS0 sender pinguim.gif\n");
}

void init_stats(){
  stats=malloc(sizeof(s_stats));
  stats->FER = 5;
  stats->t_prop = 10*1000;
  stats->baudrate = B38400;
  stats->chunk_size = 100;
  stats->R = 0;
}

int main(int argc, char** argv){
  //check for argc
  if (argc != 3 && argc != 4) {
    printf("Invalid number of arguments.\n");
    print_usage();
    return 1;
  }

  //check for argument serial port
  if (!(strcmp("/dev/ttyS0", argv[1])==0)
    || (strcmp("/dev/ttyS1", argv[1])==0)){
    printf("Invalid serialport.\n");
    print_usage();
    return 2;
  }

  //check for argument sender / receiver
  if ((strcmp("receiver", argv[2]) == 0)){
    init_stats();
    init_alarm();
    receiver(argv[1]);
    de_init_alarm();
  }
  else if ((strcmp("sender", argv[2]) == 0)){
    //check for sender's file arg
    if (argc != 4){
      printf("Invalid sender arguments. Needs filename of file to send.\n");
      print_usage();
      return 3;
    }
    init_stats();
    init_alarm();
    sender(argv[1], argv[3]);
    de_init_alarm();
  }
  else {
    printf("Invalid sender/receiver argument.\n");
    print_usage();
    return 4;
  }

  //done.
  return 0;
}
