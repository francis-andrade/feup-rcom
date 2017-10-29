#include "alarm.h"
#include "datalink.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

// timeout flag
//int timeout_flag = 0;
// durations and tries initialized in arm_alarm()

/*static unsigned char * s_frame;
static int s_fd;*/

s_alarm * alm;

void alarm_handler(int signal){
  alm->count++;
  alm->timeout_flag = 1;
  if (alm->count < alm->retries){
    printf("Timeout! Resending frame..\n");
    
    //send_frame(s_frame, s_fd);
    alarm(alm->duration);
  } else {
    
    printf("Timeout x3! Exiting..\n");
  }
}

void init_alarm(){
  struct sigaction action;
  action.sa_handler = alarm_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  //set alarm action
  sigaction(SIGALRM, &action, NULL);
}

void arm_alarm(int duration, int retries/*, int fd, unsigned char * frame*/){
  alm=malloc(sizeof(s_alarm));
  // init flag + statics
  alm->timeout_flag = 0;
  alm->duration = duration;
  alm->retries = retries;
  alm->count = 0;
  // arm it!
  /*s_frame=frame;
  s_fd=fd;
  send_frame(frame, fd);*/
  alarm(alm->duration);
}

void disarm_alarm(){
  free(alm);
  alarm(0);
}
