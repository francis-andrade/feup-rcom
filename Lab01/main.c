#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include "applicationlayer.h"
#include "datalink.h"
#include "alarm.h"


void print_usage(){
  printf("Usage:\tnserial SerialPort receiver OR nserial SerialPort sender filename \n\tex: nserial /dev/ttyS1 receiver \n\tex: nserial /dev/ttyS0 sender pinguim.gif\n");
}

int main(int argc, char** argv){
  //init alarm
  init_alarm();
  
  //check for argc
  if (argc != 3 && argc != 4) {
    printf("Invalid number of arguments.\n");
    print_usage();
    de_init_alarm();
    return 1;
  }

  //check for argument serial port
  if (!(strcmp("/dev/ttyS0", argv[1])==0)
    || (strcmp("/dev/ttyS1", argv[1])==0)){
    printf("Invalid serialport.\n");
    print_usage();
    de_init_alarm();
    return 2;
  }

  //check for argument sender / receiver
  if ((strcmp("receiver", argv[2]) == 0)){
    receiver(argv[1]);
  }
  else if ((strcmp("sender", argv[2]) == 0)){
    //check for sender's file arg
    if (argc != 4){
      printf("Invalid sender arguments. Needs filename of file to send.\n");
      print_usage();
    de_init_alarm();
      return 3;
    }
    sender(argv[1], argv[3]);
  }
  else {
    printf("Invalid sender/receiver argument.\n");
    print_usage();
    de_init_alarm();
    return 4;
  }

  //done.
  de_init_alarm();
  return 0;
}
