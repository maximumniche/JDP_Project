#pragma once
#include <Arduino.h>
#include "stm32f3xx_hal.h"

// Audio IN:  A0 (PA0)  
// Audio OUT: A2 (PA4) 
#define SAMPLE_RATE  44100
#define BUFFER_SIZE  512

// effects state 
extern volatile int crushFactor;
extern volatile float pitchSpeed;
extern volatile float fmDepth;

static void i2s_init(void);
void SPI2_IRQHandler(void);


// raw knob values for LCD display
extern volatile uint16_t k1, k2, k3;

void audio_init();
void audioISR();

static uint16_t adc_read(ADC_TypeDef *adc, uint8_t ch);
static void adc_init(ADC_TypeDef *adc);
static void dac_init();
static inline void dac_write(uint16_t v);

void knobChanges();
