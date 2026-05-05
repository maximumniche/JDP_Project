#include <Arduino.h>
#include <HardwareTimer.h>

#define AUDIO_IN_PIN  A0   // ADC input
#define AUDIO_OUT_PIN A2   // DAC output
#define knob1         A1   // bitcrush depth knob
#define knob2         A3   // pitch shift knob

#define SAMPLE_RATE  44100
#define BUFFER_SIZE  512

// ring buffer for pitch shifting
int16_t pitchBuffer[BUFFER_SIZE];
int     writeIndex = 0;
float   readIndex  = 0.0f;

// bitcrush state
int holdCounter = 0;
int lastSample  = 0;

// knob polling (read knobs every 256 samples, ~172Hz)
int   knobDivider = 0;
int   crushFactor = 1;    // 1 = no crush, 20 = max crush
float pitchRate   = 1.0f; // 1.0 = original pitch, <1 lower, >1 higher

HardwareTimer *audioTimer;

void audioISR() {
  // read knobs every 256 samples to avoid ADC conflicts
  knobDivider++;
  if (knobDivider >= 256) {
    knobDivider = 0;
    crushFactor = map(analogRead(knob1), 0, 4095, 1, 20);
    pitchRate   = (analogRead(knob2) / 4095.0f) * 2.0f; // 0.0 ~ 2.0
  }

  int input = analogRead(AUDIO_IN_PIN);

  // bitcrush: hold the same sample for crushFactor cycles
  holdCounter++;
  if (holdCounter >= crushFactor) {
    holdCounter = 0;
    lastSample  = input;
  }

  // pitch shift: write current sample into ring buffer
  pitchBuffer[writeIndex] = lastSample;
  writeIndex = (writeIndex + 1) % BUFFER_SIZE;

  // advance read pointer at pitchRate speed
  readIndex += pitchRate;
  if (readIndex >= BUFFER_SIZE) readIndex -= BUFFER_SIZE;

  // linear interpolation between two adjacent samples
  int   idx0 = (int)readIndex % BUFFER_SIZE;
  int   idx1 = (idx0 + 1) % BUFFER_SIZE;
  float frac = readIndex - (int)readIndex;
  int   out  = (int)(pitchBuffer[idx0] * (1.0f - frac) + pitchBuffer[idx1] * frac);

  analogWrite(AUDIO_OUT_PIN, constrain(out, 0, 4095));
}

void setup() {
  analogReadResolution(12);
  analogWriteResolution(12);

  // trigger audioISR at fixed 44100Hz using TIM2
  audioTimer = new HardwareTimer(TIM2);
  audioTimer->setOverflow(SAMPLE_RATE, HERTZ_FORMAT);
  audioTimer->attachInterrupt(audioISR);
  audioTimer->resume();
}

void loop() {
}