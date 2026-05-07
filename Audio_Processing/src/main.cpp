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

HardwareTimer *audioTimer;

void setup() {
    Serial.begin(9600);
    Serial1.begin(9600);

    audio_init();
    encoder_init();
    lpf_init();
    lcd_setup();

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

    // convert raw knob values to 0~100 for display
    float pitchVal = constrain((4095.0f - k2_raw - 100) * 100 / 3900, 0.0f, 100.0f);
    float srVal    = constrain((4095.0f - k1_raw)       * 100 / 3900, 0.0f, 100.0f);
    float modVal   = constrain((4095.0f - k3_raw)       * 100 / 3900, 0.0f, 100.0f);

    lcd_update(pitchVal, srVal, modVal);

    delay(50);
}
