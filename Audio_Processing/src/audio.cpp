#include "audio.h"
#include "keyboard.h"
#include "sample.h"
#include <HardwareTimer.h>
#include <math.h>

//  globals
int16_t buffer[BUFFER_SIZE];
volatile int writeIndex = 0;
float readIndex = 0.0f;

volatile int crushFactor   = 1;
int holdCounter = 0;
volatile int16_t crushedSample = 0;

volatile uint16_t k1, k2, k3;

volatile float pitchSpeed = 1.0f;
volatile float lfoPhase   = 0.0f;
volatile float fmDepth    = 0.0f;


//  ADC
static uint16_t adc_read(ADC_TypeDef *adc, uint8_t ch) {
    adc->SQR1 = (adc->SQR1 & ~ADC_SQR1_SQ1_Msk) |
                ((ch & 0x1F) << ADC_SQR1_SQ1_Pos);
    adc->CR |= ADC_CR_ADSTART;
    while (!(adc->ISR & ADC_ISR_EOC));
    return adc->DR & 0x0FFF;
}

static void adc_init(ADC_TypeDef *adc) {
    if (adc->CR & ADC_CR_ADEN) {
        adc->CR |= ADC_CR_ADDIS;
        while (adc->CR & ADC_CR_ADEN);
    }

    adc->ISR = 0xFFFFFFFF;

    adc->CR &= ~ADC_CR_ADVREGEN_Msk;
    adc->CR |= ADC_CR_ADVREGEN_0;
    delayMicroseconds(20);

    adc->CR |= ADC_CR_ADCAL;
    while (adc->CR & ADC_CR_ADCAL);

    adc->CFGR  = 0;
    adc->SMPR1 = 0x3FFFFFFF;
    adc->SMPR2 = 0x3FFFFFFF;
    adc->SQR1  = 0;

    adc->CR |= ADC_CR_ADEN;
    while (!(adc->ISR & ADC_ISR_ADRDY));
}

//  DAC
static void dac_init() {
    RCC->APB1ENR |= RCC_APB1ENR_DAC1EN;
    RCC->AHBENR  |= RCC_AHBENR_GPIOAEN;

    GPIOA->MODER |= (0x3 << (4 * 2));   // PA4 = A2 = DAC out
    GPIOA->PUPDR &= ~(0x3 << (4 * 2));

    DAC->CR = DAC_CR_EN1 | DAC_CR_BOFF1;
}

static inline void dac_write(uint16_t v) {
    DAC->DHR12R1 = v & 0x0FFF;
}

//  init
void audio_init() {
    // GPIO + ADC clocks
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_ADC12EN;

    // analog pins
    GPIOA->MODER |= (0x3 << (0 * 2));  // A0  audio in
    GPIOA->MODER |= (0x3 << (1 * 2));  // A1  bitcrush knob
    GPIOA->MODER |= (0x3 << (4 * 2));  // A4  pitch knob
    GPIOA->MODER |= (0x3 << (5 * 2));  // A5  FM knob

    ADC12_COMMON->CCR = ADC12_CCR_CKMODE_0;

    adc_init(ADC1);
    dac_init();
}

// Knob modifications
void knobChanges() {
    noInterrupts();
    k1 = adc_read(ADC1, 2);  // bitcrush  A1 ch2
    k2 = adc_read(ADC1, 7);  // pitch     A4 ch7
    k3 = adc_read(ADC1, 6);  // FM        A5 ch6

    crushFactor = (k1 * 19) / 4095 + 1;
    pitchSpeed  = 0.5f + (k2 / 4095.0f) * 1.5f;
    fmDepth     = (k3 / 4095.0f) * 0.8f;
    interrupts();
}

//  ISR
void audioISR() {

    /* --------------------------- // Live audio mods --------------------------- */
    // audio input (ch1 = PA0 = A0)
    int input = adc_read(ADC1, 1);
    // int16_t centered = (int16_t)input - 2048;

    // bitcrush
    holdCounter++;
    if (holdCounter >= crushFactor) {
        holdCounter   = 0;
        // crushedSample = centered;
        crushedSample = input;
    }

    // write ring buffer
    buffer[writeIndex] = crushedSample;
    writeIndex = (writeIndex + 1) % BUFFER_SIZE;

    // LFO (5 Hz sine)
    lfoPhase += (5.0f / SAMPLE_RATE);
    if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
    float lfo = sinf(lfoPhase * 6.28318f);
    // float lfo = (lfoPhase < 0.5f) ? (lfoPhase * 4.0f - 1.0f) : (3.0f - lfoPhase * 4.0f);

    // pitch + FM modulation
    float modSpeed = pitchSpeed + lfo * fmDepth;
    if (modSpeed < 0.2f) modSpeed = 0.2f;
    if (modSpeed > 2.5f) modSpeed = 2.5f;

    // read ring buffer with interpolation
    readIndex += modSpeed;
    if (readIndex >= BUFFER_SIZE) readIndex -= BUFFER_SIZE;

    int   i0   = (int)readIndex;
    int   i1   = (i0 + 1) % BUFFER_SIZE;
    float frac = readIndex - i0;

    float liveOut = buffer[i0] * (1.0f - frac) + buffer[i1] * frac;
    // out += 2048.0f;

    /* ----------------------------- // Sample audio ---------------------------- */

    // --- sample playback path ---
    int16_t sampleOut = 0;
    if (samplePlaying) {
        sampleOut = sample[sampleIndex] - 2048;  // center it
        sampleIndex++;
        if (sampleIndex >= SAMPLE_LENGTH) {
            sampleIndex = 0;
            samplePlaying = false;  // or loop: sampleIndex = 0
        }
    }

    // --- mix ---
    int32_t mixed = ((int32_t)(liveOut) + sampleOut) / 2;
    if (mixed > 4095) mixed = 4095;
    if (mixed < 0)    mixed = 0;
    dac_write((uint16_t)mixed);

}
