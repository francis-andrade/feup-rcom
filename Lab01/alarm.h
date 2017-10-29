#ifndef ALARM_H
#define ALARM_H

#include <stdio.h>
#include <unistd.h>
#include <signal.h>

typedef struct {
  int timeout_flag;
  int duration;
  int retries;
  int count;
  int status;
} s_alarm;



void init_alarm();
void arm_alarm(int duration, int retries);
void disarm_alarm();

extern s_alarm * alm;



#endif
