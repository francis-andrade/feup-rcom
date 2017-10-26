#ifndef ALARM_H
#define ALARM_H

#include "utils.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#define TIMEOUT_SECS 3 // TODO change this to linklayer's timeout
#define MAX_TIMEOUTS 3

void init_alarm();
void arm_alarm(function);
void disarm_alarm();

#endif