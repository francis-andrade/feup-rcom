#include "alarm.h"

volatile int n_timeouts = 0;
volatile int timeoutFlag = 0;

void alarm_handler(int signal){
  if (signal != SIGALRM)
    return;

  n_timeouts++;

  if (n_timeouts < MAX_TIMEOUTS){
    printf("Timeout! Resending frame..");
    timeoutFlag = 1;
    alarm(TIMEOUT_SECS);
  } else {
    printf("Timeout x3! Exiting..");
  }
}

void init_alarm(){
  struct sigaction action;
  action.sa_handler = alarm_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;

  sigaction(SIGALRM, &action, NULL);
}

void arm_alarm(function){
  n_timeouts = 0;
  timeoutFlag = 0;
  alarm(TIMEOUT_SECS);
}

void disarm_alarm(){
  alarm(0);
}
