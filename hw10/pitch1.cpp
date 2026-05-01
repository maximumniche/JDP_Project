#include <Arduino.h>
#define AUDIO_IN_PIN A0   // ADC 
#define AUDIO_OUT_PIN A2  // DAC 
#define knob1 A1 // knob for bitcrushing
#define knob2 A3 // knob for pitch shifting

// bitcrush 
int holdCounter = 0;
int lastSample = 0;

// pitch buffer 
const int bufferSize = 512;
uint16_t buffer[bufferSize];
int writeIndex = 0;
float readIndex = 0;

// timing
unsigned long lastMicros = 0;
const int sampleInterval = 50; // ~20kHz

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
  float pitch = map(knobPitch, 0, 4095, 50, 200) / 100.0;
  int index = (int)readIndex % bufferSize;
  uint16_t output = buffer[index];
  readIndex += pitch;
  if (readIndex >= bufferSize) readIndex -= bufferSize;


  //analogWrite(AUDIO_OUT_PIN, output);
  //analogWrite(AUDIO_OUT_PIN, input);
  analogWrite(AUDIO_OUT_PIN, lastSample);

}

