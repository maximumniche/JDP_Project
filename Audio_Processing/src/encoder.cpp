#include "encoder.h"

volatile int  currentStep = 0;
volatile bool stepDirty   = false;

static volatile int encLastA = HIGH;

void encoderISR() {
    static uint32_t lastTime = 0;
    uint32_t now = micros();
    if (now - lastTime < 10000) return;  // ignore pulses within 5ms
    lastTime = now;

    int a = digitalRead(DT_PIN);
    int b = digitalRead(CLK_PIN);

    if (a == LOW) {
        if (b == HIGH) currentStep = constrain(currentStep + 1, 0, 15);
        else           currentStep = constrain(currentStep - 1, 0, 15);
        stepDirty = true;
    }
}

void encoder_init() {
    pinMode(CLK_PIN, INPUT_PULLUP);
    pinMode(DT_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(CLK_PIN), encoderISR, CHANGE);
}
