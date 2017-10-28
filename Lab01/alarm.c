#include "alarm.h"

// timeout flag
int timeout_flag = 0;
// durations and tries initialized in arm_alarm()
static int s_duration = 0;
static int s_retries = 0;
// count iterated in alarm_handler()
static int s_count = 0;

void alarm_handler(int signal){
  s_count++;

  if (s_count < s_retries){
    printf("Timeout! Resending frame..");
    timeout_flag = 1;
    alarm(s_duration);
  } else {
    printf("Timeout x3! Exiting..");
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

void arm_alarm(int duration, int retries){
  // init flag + statics
  timeout_flag = 0;
  s_duration = duration;
  s_retries = retries;
  s_count = 0;
  // arm it!
  alarm(s_duration);
}

void disarm_alarm(){
  alarm(0);
}
