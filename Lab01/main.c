#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include "applicationlayer.h"
#include "datalink.h"
#include "alarm.h"
#include "utils.h"


void print_usage(){
  printf("Usage:\n");
  printf("\t./nserial port baudrate max_retries timeout mode [filename]");
  printf("Examples:\n");
  printf("\t./nserial /dev/ttyS0 B38400 3 3 sender pinguim.gif\n");
  printf("\t./nserial /dev/ttyS1 B2400 3 3 receiver \n");
}


int main(int argc, char** argv){  
  //check for argc
  if (argc != 6 && argc != 7) {
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

  //check for baudrate
  int baud = get_baudrate(argv[2]);
  if (baud<0){
    printf("Invalid baudrate. Pick a value from among the following: {B0, B50, B75, B110, B134, B150, B200, B300, B600, B1200, B1800, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000}\n");
    print_usage();
    return 4;
  }

  //check for max_retries
  int max_retries = atoi(argv[3]);
  if (max_retries<0){
    printf("Invalid max retries. It must be greater than 0\n");
    print_usage();
    return 4;
  }

  //check for timeout
  int timeout = atoi(argv[4]);
  if (timeout<0){
    printf("Invalid timeout. It must be greater than 0\n");
    print_usage();
    return 5;
  }

  //check for argument sender / receiver
  if ((strcmp("receiver", argv[5]) == 0)){
    return receiver(argv[1], baud, max_retries, timeout);
  }
  else if ((strcmp("sender", argv[5]) == 0)){
    //check for sender's file arg
    if (argc != 7){
      printf("Invalid sender arguments. Needs filename of file to send.\n");
      print_usage();
      return 6;
    }
    return sender(argv[1], baud, max_retries, timeout, argv[6]);
  }
  else {
    printf("Invalid sender/receiver argument.\n");
    print_usage();
    return 7;
  }

  //done.
  return 0;
}
