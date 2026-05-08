#pragma once
#include <Arduino.h>
#include <Wire.h>

#define KB_ADDR 0x5F

extern volatile bool samplePlaying;
extern volatile int sampleIndex;
extern volatile int sampleNum;

void keyboard_init();
void get_keyboard_data();
