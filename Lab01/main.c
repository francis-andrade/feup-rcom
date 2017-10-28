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
  Applicationlayer app;
  
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
    app.status = RECEIVER;
  }
  else if ((strcmp("sender", argv[2]) == 0)){
    app.status = SENDER;
    //check for sender's file arg
    if (argc != 4){
      printf("Invalid sender arguments. Needs filename of file to send.\n");
      print_usage();
      return 3;
    }
  }
  else {
    printf("Invalid sender/receiver argument.\n");
    print_usage();
    return 4;
  }

  //open serial port
  app.filedescriptor = open_port(argv[1]);
  //init alarm
  init_alarm();

  //enter sender/receiver process
  if (app.status == SENDER){
    sender(app, argv[1], argv[3]);
  } else {
    receiver(app, argv[1]);
  }

  //done.
  return 0;
}
