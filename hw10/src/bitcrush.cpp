


#include <Arduino.h>

#define AUDIO_IN_PIN A0
#define AUDIO_OUT_PIN D9

int bitDepth = 8;
int crushFactor = 4;

int holdCounter = 0;
int lastSample = 0;

unsigned long lastMicros = 0;
const int sampleInterval = 50; // 20kHz ≈ 50us

void setup() {
  analogReadResolution(12);
  analogWriteResolution(12);

  Serial.begin(115200);
}

void loop() {
  // timing control (software sample rate)
  if (micros() - lastMicros >= sampleInterval) {
    lastMicros = micros();

    int input = analogRead(AUDIO_IN_PIN);
    
    Serial.println(analogRead(A0));

    // sample rate reduction
    holdCounter++;
    if (holdCounter >= crushFactor) {
      holdCounter = 0;

      int shift = 12 - bitDepth;
      lastSample = (input >> shift) << shift;
    }

    analogWrite(AUDIO_OUT_PIN, lastSample);
  }

  // knobs
  int knob1 = analogRead(A1);
  int knob2 = analogRead(A2);

  bitDepth = map(knob1, 0, 4095, 4, 12);
  crushFactor = map(knob2, 0, 4095, 1, 20);
}