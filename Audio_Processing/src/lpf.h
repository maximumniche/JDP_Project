#pragma once
#include <Arduino.h>

#define pwm1 D9
#define pwm2 D10

extern const int cutoffFreq[16];
extern const int pwm1value[16];
extern const int pwm2value[16];

void lpf_init();
void lpf_update(int step);   // call when encoder moves
