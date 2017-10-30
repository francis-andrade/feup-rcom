#include "alarm.h"
#include "datalink.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

s_alarm alm;

void alarm_handler(int signal){
  alm.count++;
  alm.timeout_flag = 1;
  if (alm.count < alm.retries){
    printf("Timeout! Resending frame..\n");
    alarm(alm.duration);
  } else {
    
    printf("Timeout x3! Exiting..\n");
  }
}

void init_alarm(int duration, int retries){
  alm.count=0;
  alm.timeout_flag=0;
  alm.duration = duration;
  alm.retries = retries;
  struct sigaction action;
  action.sa_handler = alarm_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  //set alarm action
  sigaction(SIGALRM, &action, NULL);
}

void arm_alarm(){
  // init flag + statics
  alm.timeout_flag = 0;
  alm.count = 0;
  // arm it!
  alarm(alm.duration);
}

void disarm_alarm(){
  alarm(0);
}
