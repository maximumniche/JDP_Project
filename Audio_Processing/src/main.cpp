// A1  bitcrush knob
// A4  pitch shift knob
// A5  FM depth knob
// A0  audio in  
// A2  audio out 
// D9  PWM1, D10 PWM2
// D3  encoder CLK, D4 encoder DT
// D8  Serial1 TX → LCD

#include <Arduino.h>
#include <HardwareTimer.h>
#include "audio.h"
#include "encoder.h"
#include "lpf.h"
#include "lcd.h"
#include "keyboard.h"

HardwareTimer *audioTimer;

void setup() {
    Serial.begin(9600);
    Serial1.begin(9600);

    audio_init();
    encoder_init();
    lpf_init();
    lcd_setup();
    keyboard_init();

    Serial.println("Ready");

    audioTimer = new HardwareTimer(TIM2);
    audioTimer->setOverflow(SAMPLE_RATE, HERTZ_FORMAT);
    audioTimer->attachInterrupt(audioISR);
    audioTimer->resume();
}

void loop() {
    // update PWM if encoder moved
    if (stepDirty) {
        stepDirty = false;
        lpf_update(currentStep);
        Serial.print("Cutoff: ");
        Serial.println(cutoffFreq[currentStep]);

    }

    knobChanges();

    // Display knob values based on what they are
    float pitchVal = pitchSpeed;
    float srVal = crushFactor;
    float modVal = fmDepth;

    lcd_update(pitchVal, srVal, modVal);

    get_keyboard_data();

    delay(50);
}
