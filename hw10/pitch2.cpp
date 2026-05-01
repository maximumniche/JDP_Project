#include <Arduino.h>

#define AUDIO_IN_PIN A0
#define AUDIO_OUT_PIN A2
#define KNOB_CRUSH A1
#define KNOB_PITCH A3

//bitcrush 
int holdCounter = 0;
int lastSample = 0;

// pitch buffer
const int bufferSize = 1024;
uint16_t buffer[bufferSize];
int writeIndex = 0;
float readIndex = 0;

//timing
unsigned long lastMicros = 0;
const int sampleInterval = 50; // 20kHz

void setup() {
  analogReadResolution(12);
  analogWriteResolution(12);
}

void loop() {

  if (micros() - lastMicros >= sampleInterval) {
    lastMicros += sampleInterval;

    int input = analogRead(AUDIO_IN_PIN);

    //  bitcrush 
    int knobCrush = analogRead(KNOB_CRUSH);
    int crushFactor = map(knobCrush, 0, 4095, 1, 20);

    holdCounter++;
    if (holdCounter >= crushFactor) {
      holdCounter = 0;
      lastSample = input;
    }

    //  pitch 
    buffer[writeIndex] = lastSample;
    writeIndex = (writeIndex + 1) % bufferSize;

    int knobPitch = analogRead(KNOB_PITCH);
    float pitch = map(knobPitch, 0, 4095, 70, 140) / 100.0;
    int index0 = (int)readIndex;
    int index1 = (index0 + 1) % bufferSize;
    float frac = readIndex - index0;
    uint16_t s0 = buffer[index0];
    uint16_t s1 = buffer[index1];
    uint16_t output = (1 - frac) * s0 + frac * s1;

    readIndex += pitch;
    if (readIndex >= bufferSize) readIndex -= bufferSize;

    analogWrite(AUDIO_OUT_PIN, output);
  }
}