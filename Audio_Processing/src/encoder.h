#pragma once
#include <Arduino.h>

// CLK → D3, DT → D4
#define ENC_A D3
#define ENC_B D4

extern volatile int  currentStep;
extern volatile bool stepDirty;   // true when encoder moved, cleared in loop()

void encoder_init();
void encoderISR();
