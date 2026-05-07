#pragma once
#include <Arduino.h>
#include "stm32f3xx_hal.h"

// Audio IN:  A0 (PA0)  
// Audio OUT: A2 (PA4) 
#define SAMPLE_RATE  44100
#define BUFFER_SIZE  512

// effects state 
extern int     crushFactor;
extern float   pitchSpeed;
extern float   fmDepth;

// raw knob values for LCD display
extern volatile uint16_t k1_raw, k2_raw, k3_raw;

void     audio_init();
void     audioISR();

static uint16_t adc_read(ADC_TypeDef *adc, uint8_t ch);
static void     adc_init(ADC_TypeDef *adc);
static void     dac_init();
static inline void dac_write(uint16_t v);
