#include <Arduino.h>
#include <HardwareTimer.h>

#define AUDIO_IN_PIN  A0   // ADC input
#define AUDIO_OUT_PIN A2   // DAC output
#define knob1         A1   // bitcrush depth knob
#define knob2         A3   // pitch shift knob
#define knob3         A4   // FM depth knob

#define SAMPLE_RATE  44100
#define BUFFER_SIZE  512

// precomputed sine table (256 steps, 0~255 range centered at 0)
static int8_t sineTable[256];

// ring buffer for pitch shifting
int16_t pitchBuffer[BUFFER_SIZE];
int     writeIndex = 0;
float   readIndex  = 0.0f;

// bitcrush state
int holdCounter = 0;
int lastSample  = 0;

// knob values
int   knobDivider = 0;
int   crushFactor = 1;
float pitchRate   = 1.0f;
float fmDepth     = 0.0f;  // how much LFO wobbles the read speed

// LFO state (fixed ~5Hz wobble)
float lfoPhase     = 0.0f;
const float LFO_HZ = 5.0f;
const float LFO_INC = LFO_HZ / SAMPLE_RATE; // phase increment per sample

HardwareTimer *audioTimer;

void audioISR() {
  // read knobs every 256 samples (~172Hz)
  knobDivider++;
  if (knobDivider >= 256) {
    knobDivider = 0;
    crushFactor = map(analogRead(knob1), 0, 4095, 1, 20);
    pitchRate   = (analogRead(knob2) / 4095.0f) * 2.0f;  // 0.0 ~ 2.0
    fmDepth     = (analogRead(knob3) / 4095.0f) * 0.8f;  // 0.0 ~ 0.8
  }

  int input = analogRead(AUDIO_IN_PIN);

  // bitcrush: hold sample for crushFactor cycles
  holdCounter++;
  if (holdCounter >= crushFactor) {
    holdCounter = 0;
    lastSample  = input;
  }

  // advance LFO phase
  lfoPhase += LFO_INC;
  if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;

  // lookup sine from table (-1.0 ~ +1.0)
  int   lfoIdx  = (int)(lfoPhase * 256) & 0xFF;
  float lfoSine = sineTable[lfoIdx] / 127.0f;

  // modulate read speed: pitchRate ± fmDepth
  float modulatedRate = pitchRate + lfoSine * fmDepth;

  // write into ring buffer
  pitchBuffer[writeIndex] = lastSample;
  writeIndex = (writeIndex + 1) % BUFFER_SIZE;

  // advance read pointer with modulated rate
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

  // precompute sine table once
  for (int i = 0; i < 256; i++) {
    sineTable[i] = (int8_t)(sinf(2.0f * PI * i / 256.0f) * 127.0f);
  }

  // trigger audioISR at fixed 44100Hz
  audioTimer = new HardwareTimer(TIM2);
  audioTimer->setOverflow(SAMPLE_RATE, HERTZ_FORMAT);
  audioTimer->attachInterrupt(audioISR);
  audioTimer->resume();
}

void loop() {
}