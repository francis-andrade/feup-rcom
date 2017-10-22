#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>

void invalid_parameters(){
  printf("Usage:\tnserial SerialPort receiver OR nserial SerialPort sender filename
          \n\tex: nserial /dev/ttyS1 receiver
          \n\tex: nserial /dev/ttyS0 sender pinguim.gif"
        );
  exit(1);
}

int main(int argc, char** argv){

  unsigned char buf[255];
  int ROLE;

  if (argc > 1 && ((strcmp("/dev/ttyS0", argv[1]) == 0) || (strcmp("/dev/ttyS1", argv[1]) == 0))){
    if((argc == 3 && (strcmp("receiver", argv[2])!=0)) && )
      ROLE = 1;
    else if((argc == 4 && (strcmp("sender", argv[2])!=0)))
      ROLE = 0;
    else invalid_parameters();
  } else invalid_parameters();

  if (ROLE == 0){
    sender(argv[3]);
  } else {
    receiver();
  }

  return 0;
}
