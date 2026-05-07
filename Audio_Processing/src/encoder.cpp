#include "encoder.h"

volatile int  currentStep = 0;
volatile bool stepDirty   = false;

static volatile int encLastA = HIGH;

void encoderISR() {
    int a = digitalRead(ENC_A);
    int b = digitalRead(ENC_B);

    if (a != encLastA) {
        encLastA = a;
        if (a == LOW) {
            if (b == HIGH) currentStep = constrain(currentStep + 1, 0, 15);
            else           currentStep = constrain(currentStep - 1, 0, 15);
            stepDirty = true;
        }
    }
}

void encoder_init() {
    pinMode(ENC_A, INPUT_PULLUP);
    pinMode(ENC_B, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENC_A), encoderISR, CHANGE);
}
