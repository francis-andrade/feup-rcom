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

extern s_alarm alm;



void init_alarm(int duration, int retries);
void arm_alarm();
void disarm_alarm();
void deinit_alarm();



#endif
