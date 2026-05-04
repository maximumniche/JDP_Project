#include <Arduino.h>
#include <HardwareTimer.h>
#include <cmath>

//  PWM LPF pins & button 
#define pwm1   D9
#define pwm2   D10
#define BUTTON D8

//  audio pins 
#define AUDIO_IN_PIN  A0
#define AUDIO_OUT_PIN A2
#define knob1         A1   // bitcrush depth
#define knob2         A3   // pitch shift
#define knob3         A4   // FM depth

//  PWM LPF tables (16 steps) 
const int cutoffFreq[16] = {12000, 8721, 6338, 4606,
                             3348,  2433, 1768, 1285,
                             934,   679,  493,  358,
                             261,   189,  138,  100};

const int pwm1value[16] = {1556, 1064, 737, 491,
                            348,  266,  184, 143,
                            110,  90,   72,  61,
                            53,   47,   43,  40};

const int pwm2value[16] = {1064, 778, 532, 368,
                            286,  225, 163, 131,
                            102,  86,  71,  62,
                            54,   49,  45,  42};

int currentStep    = 0;
int prevButtonState = HIGH;

//  audio / DSP 
#define SAMPLE_RATE  44100
#define BUFFER_SIZE  512

static int8_t sineTable[256];

// pitch shift ring buffer
int16_t pitchBuffer[BUFFER_SIZE];
int     writeIndex = 0;
float   readIndex  = 0.0f;

// bitcrush state
int holdCounter = 0;
int lastSample  = 0;

// knob cached values (updated inside ISR every 256 samples)
int   knobDivider = 0;
int   crushFactor = 1;
float pitchRate   = 1.0f;
float fmDepth     = 0.0f;

// LFO (fixed 5Hz)
float lfoPhase      = 0.0f;
const float LFO_INC = 5.0f / SAMPLE_RATE;

HardwareTimer *audioTimer;


//  Audio ISR — runs at 44100 Hz
void audioISR() {
  // read all three knobs every 256 samples (~172Hz)
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

  // LFO sine
  lfoPhase += LFO_INC;
  if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
  float lfoSine = sineTable[(int)(lfoPhase * 256) & 0xFF] / 127.0f;

  // pitch shift + FM: modulate read rate
  float modulatedRate = pitchRate + lfoSine * fmDepth;

  // write into ring buffer
  pitchBuffer[writeIndex] = lastSample;
  writeIndex = (writeIndex + 1) % BUFFER_SIZE;

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
  Serial.begin(115200);

  //  PWM LPF setup 
  pinMode(BUTTON, INPUT_PULLUP);
  analogWriteResolution(12);
  analogWriteFrequency(17578);   // 17.578 kHz PWM (global)
  analogWrite(pwm1, pwm1value[currentStep]);
  analogWrite(pwm2, pwm2value[currentStep]);
  Serial.print("Cutoff Frequency(Hz): ");
  Serial.println(cutoffFreq[currentStep]);

  analogReadResolution(12);

  // precompute sine table
  for (int i = 0; i < 256; i++) {
    sineTable[i] = (int8_t)(sinf(2.0f * PI * i / 256.0f) * 127.0f);
  }

  // start audio timer at 44100 Hz
  audioTimer = new HardwareTimer(TIM2);
  audioTimer->setOverflow(SAMPLE_RATE, HERTZ_FORMAT);
  audioTimer->attachInterrupt(audioISR);
  audioTimer->resume();
}

// pwm control
void loop() {
  int buttonState = digitalRead(BUTTON);
  if (buttonState == LOW && prevButtonState == HIGH) {
    currentStep = (currentStep + 1) % 16;
    analogWrite(pwm1, pwm1value[currentStep]);
    analogWrite(pwm2, pwm2value[currentStep]);
    Serial.print("Cutoff Frequency(Hz): ");
    Serial.println(cutoffFreq[currentStep]);
  }

  prevButtonState = buttonState;
  delay(50);  
}