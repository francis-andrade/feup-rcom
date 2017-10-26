#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include "applicationlayer.h"
#include "datalink.h"

void invalid_parameters(int i){
  switch(i){
    case 0:
      printf("Invalid number of arguments. ");
    break;
    case 1:
      printf("Invalid sender/receiver arguments. ");
    break;
    default:
      printf("Invalid port arguments. ");
  };

  printf("Usage:\tnserial SerialPort receiver OR nserial SerialPort sender filename \n\tex: nserial /dev/ttyS1 receiver \n\tex: nserial /dev/ttyS0 sender pinguim.gif\n");
  
  exit(1);
}

int main(int argc, char** argv){
  Applicationlayer app;

  if (argc < 1)
    invalid_parameters(0);
  else {
    if ((strcmp("/dev/ttyS0", argv[1]) == 0) || (strcmp("/dev/ttyS1", argv[1]) == 0)) {
      if ((argc == 3 && (strcmp("receiver", argv[2]) == 0)))
        app.status = RECEIVER;
      else if ((argc == 4 && (strcmp("sender", argv[2]) == 0)))
        app.status = SENDER;
      else
        invalid_parameters(1);
    }
    else
      invalid_parameters(2);
  }

  app.filedescriptor = open_port(argv[1]);

  if (app.status == SENDER){
    sender(app, argv[1], argv[3]);
  } else {
    receiver(app, argv[1]);
  }

  return 0;
}
