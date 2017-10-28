#ifndef ALARM_H
#define ALARM_H

#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int timeout_flag;

void init_alarm();
void arm_alarm(int duration, int retries);
void disarm_alarm();


#endif