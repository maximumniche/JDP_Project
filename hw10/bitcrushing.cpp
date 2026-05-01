#include <Arduino.h>
#define AUDIO_IN_PIN A0   // ADC 
#define AUDIO_OUT_PIN A2  // DAC 
#define knob1 A1 // knob for bitcrushing

// bitcrush 
int holdCounter = 0;
int lastSample = 0;

void setup() {
  analogReadResolution(12);
  analogWriteResolution(12);
}

void loop() {
  int input = analogRead(AUDIO_IN_PIN);

  // bitcrush
  int knobCrush = analogRead(knob1);
  int crushFactor = map(knobCrush, 0, 4095, 1, 20);
  holdCounter++;
  if (holdCounter >= crushFactor) {
      holdCounter = 0;
      lastSample = input;
    }
  //analogWrite(AUDIO_OUT_PIN, input);
  analogWrite(AUDIO_OUT_PIN, lastSample);
}