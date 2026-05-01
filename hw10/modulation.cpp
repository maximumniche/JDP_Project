#include <Arduino.h>
#include <math.h>

#define AUDIO_IN_PIN A0
#define AUDIO_OUT_PIN A2
#define knob1 A1
#define knob2 A3 // for modulation

int holdCounter = 0;
int lastSample = 0;

float phase = 0;

void setup() {
  analogReadResolution(12);
  analogWriteResolution(12);
}

void loop() {

  int input = analogRead(AUDIO_IN_PIN);
  int centered = input - 2048;
  // bit crush
  int knobCrush = analogRead(knob1);
  int crushFactor = map(knobCrush, 0, 4095, 1, 15);

  holdCounter++;
  if (holdCounter >= crushFactor) {
    holdCounter = 0;
    lastSample = centered;
  }

  //  modulation control
  int knobMod = analogRead(knob2);

  float freq = map(knobMod, 0, 4095, 0, 30); 
  float depth = knobMod / 4095.0; 
  phase += (2.0 * PI * freq) / 20000.0;
  if (phase > 2 * PI) phase -= 2 * PI;

  float lfo = sin(phase);
  float gain = 1.0 + (lfo * 0.8 * depth); 
  float out = lastSample * gain;

  
  int output = (int)(out + 2048);
  if (output < 0) output = 0;
  if (output > 4095) output = 4095;

  analogWrite(AUDIO_OUT_PIN, output);
}