#ifndef ALARM_H
#define ALARM_H

#include "utils.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

void init_alarm();
void arm_alarm(function);
void disarm_alarm();

#endif