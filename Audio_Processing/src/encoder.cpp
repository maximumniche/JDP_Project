#include "encoder.h"

volatile int  currentStep = 0;
volatile bool stepDirty   = false;

static volatile int encLastA = HIGH;

void encoderISR() {
    int a = digitalRead(CLK_PIN);
    int b = digitalRead(DT_PIN);

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
    pinMode(CLK_PIN, INPUT_PULLUP);
    pinMode(DT_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(CLK_PIN), encoderISR, FALLING);
}
