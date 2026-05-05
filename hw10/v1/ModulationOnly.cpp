#include <Arduino.h>
#include <HardwareTimer.h>

#define AUDIO_IN_PIN  A0
#define AUDIO_OUT_PIN A2
#define knob1         A1   // FM depth

#define SAMPLE_RATE  44100
#define BUFFER_SIZE  512

static int8_t sineTable[256];

int16_t pitchBuffer[BUFFER_SIZE];
int     writeIndex = 0;
float   readIndex  = 0.0f;

int   knobDivider = 0;
float fmDepth     = 0.0f;

float lfoPhase      = 0.0f;
const float LFO_HZ  = 5.0f;
const float LFO_INC = LFO_HZ / SAMPLE_RATE;

HardwareTimer *audioTimer;

void audioISR() {
  // read knob every 256 samples
  knobDivider++;
  if (knobDivider >= 256) {
    knobDivider = 0;
    fmDepth = (analogRead(knob1) / 4095.0f) * 0.8f;  // 0.0 ~ 0.8
  }

  int input = analogRead(AUDIO_IN_PIN);

  // write into ring buffer
  pitchBuffer[writeIndex] = input;
  writeIndex = (writeIndex + 1) % BUFFER_SIZE;

  // advance LFO
  lfoPhase += LFO_INC;
  if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;

  // sine lookup (-1.0 ~ +1.0)
  int   lfoIdx  = (int)(lfoPhase * 256) & 0xFF;
  float lfoSine = sineTable[lfoIdx] / 127.0f;

  // read speed = 1.0 (normal) ± FM wobble
  float modulatedRate = 1.0f + lfoSine * fmDepth;

  // advance read pointer
  readIndex += modulatedRate;
  if (readIndex >= BUFFER_SIZE) readIndex -= BUFFER_SIZE;
  if (readIndex < 0)            readIndex += BUFFER_SIZE;

  // linear interpolation
  int   idx0 = (int)readIndex % BUFFER_SIZE;
  int   idx1 = (idx0 + 1) % BUFFER_SIZE;
  float frac = readIndex - (int)readIndex;
  int   out  = (int)(pitchBuffer[idx0] * (1.0f - frac) + pitchBuffer[idx1] * frac);

  analogWrite(AUDIO_OUT_PIN, constrain(out, 0, 4095));
}

void setup() {
  analogReadResolution(12);
  analogWriteResolution(12);

  // precompute sine table
  for (int i = 0; i < 256; i++) {
    sineTable[i] = (int8_t)(sinf(2.0f * PI * i / 256.0f) * 127.0f);
  }

  audioTimer = new HardwareTimer(TIM2);
  audioTimer->setOverflow(SAMPLE_RATE, HERTZ_FORMAT);
  audioTimer->attachInterrupt(audioISR);
  audioTimer->resume();
}

void loop() {
  // all processing in audioISR
}