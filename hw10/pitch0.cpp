#include <Arduino.h>
#define AUDIO_IN_PIN A0   // ADC 
#define AUDIO_OUT_PIN A2  // DAC 
#define knob1 A1 // knob for bitcrushing
#define knob2 A3 // knob for pitch shifting

int holdCounter = 0;
int lastSample = 0;

unsigned long lastMicros = 0;
int sampleInterval = 50; // base 20kHz

void setup() {
  analogReadResolution(12);
  analogWriteResolution(12);
}

void loop() {

  int input = analogRead(AUDIO_IN_PIN);

  int knobCrush = analogRead(knob1);
  int crushFactor = map(knobCrush, 0, 4095, 1, 20);

  int knobPitch = analogRead(knob2);
  int pitchOffset = map(knobPitch, 0, 4095, -30, 30);
  int currentInterval = sampleInterval + pitchOffset;
  if (currentInterval < 20) currentInterval = 20;   // limit
  if (currentInterval > 200) currentInterval = 200;

  if (micros() - lastMicros >= currentInterval) 
  {
    lastMicros += currentInterval;

    holdCounter++;
    if (holdCounter >= crushFactor) {
      holdCounter = 0;
      lastSample = input;
    }
    analogWrite(AUDIO_OUT_PIN, lastSample);
  }

  // holdCounter++;
  // if (holdCounter >= crushFactor) {
  //     holdCounter = 0;
  //     lastSample = input;
  //   }

  // analogWrite(AUDIO_OUT_PIN, input);
  // analogWrite(AUDIO_OUT_PIN, lastSample);
}
