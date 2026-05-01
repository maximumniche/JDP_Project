#include <Arduino.h>

#define AUDIO_IN_PIN A0
#define AUDIO_OUT_PIN A2
#define knob1 A1
#define knob2 A3   

// bitcrush 
int holdCounter = 0;
int lastSample = 0;

// buffer for pitch shifting
const int bufferSize = 512;
uint16_t buffer[bufferSize];
int writeIndex = 0;
float readIndex = 0;

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

  // pitchshift
  buffer[writeIndex] = lastSample;
  writeIndex = (writeIndex + 1) % bufferSize;
  int knobPitch = analogRead(knob2);

  float pitch = map(knobPitch, 0, 4095, 50, 200) / 100.0;   // 0.5x ~ 2.0x
  readIndex += pitch;
  if (readIndex >= bufferSize) {
    readIndex -= bufferSize;
  }
  int i0 = (int)readIndex;
  int i1 = (i0 + 1) % bufferSize;
  float frac = readIndex - i0;

  uint16_t s0 = buffer[i0];
  uint16_t s1 = buffer[i1];
  uint16_t output = (1 - frac) * s0 + frac * s1;

  analogWrite(AUDIO_OUT_PIN, output);
}