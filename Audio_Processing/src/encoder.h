#pragma once
#include <Arduino.h>

// CLK → D3, DT → D4
#define CLK_PIN D3
#define DT_PIN D4

extern volatile int  currentStep;
extern volatile bool stepDirty;   // true when encoder moved, cleared in loop()

void encoder_init();
void encoderISR();
