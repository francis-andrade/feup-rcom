#include "alarm.h"


#define TIMEOUT_SECS 3 // TODO change this to linklayer's timeout
#define MAX_TIMEOUTS 3
volatile int n_timeouts = 0;
volatile int timeoutFlag = 0;

void alarm_handler(int signal){
  if (signal != SIGALARM)
    return;

  n_timeouts++;

  if (n_timeouts < max_timeouts){
    printf("Timeout! Resending frame..")
        timeoutFlag = 1;
    alarm(timeout);
  } else {
    printf("Timeout x3! Exiting..")
  }
}

void init_alarm(){
  struct sigaction action;
  action.handler = alarm_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;

  sigaction(SIGALARM, &action, NULL);
}

void arm_alarm(function){
  n_timeouts = 0;
  timeoutFlag = 0;
  alarm(timeout);
}

void disarm_alarm(){
  alarm(0);
}
