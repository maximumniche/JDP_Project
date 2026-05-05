#include <Arduino.h>
#include <HardwareTimer.h>

// A0 = PA0 -> ADC1_IN1  (audio in)
// A2 = PA4 -> DAC1_OUT1 (audio out)

#define SAMPLE_RATE  44100
#define BUFFER_SIZE  512
#define knob1 A1   // bitcrush knob
#define knob2 A3   // pitch shift knob

HardwareTimer *audioTimer;

// bitcrush state
int crushFactor = 1;
int holdCounter = 0;
int lastSample  = 0;

// pitch shift state
int16_t pitchBuffer[BUFFER_SIZE];
int     writeIndex = 0;
float   readIndex  = 0.0f;
float   pitchRate  = 1.0f;

int knobDivider = 0;

//  ADC (bare register, audio only)
static uint16_t adc_read_ch(uint8_t ch) {
    ADC1->SQR1 = (ADC1->SQR1 & ~ADC_SQR1_SQ1_Msk) | ((ch & 0x1F) << ADC_SQR1_SQ1_Pos);
    ADC1->CR  |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC));
    return (uint16_t)(ADC1->DR & 0x0FFF);
}

static void adc_init(void) {
    if (ADC1->CR & ADC_CR_ADEN) {
        ADC1->CR |= ADC_CR_ADDIS;
        while (ADC1->CR & ADC_CR_ADEN);
    }
    ADC1->ISR = 0xFFFFFFFF;

    ADC1->CR &= ~ADC_CR_ADVREGEN_Msk;
    ADC1->CR |=  ADC_CR_ADVREGEN_0;
    delayMicroseconds(20);

    ADC1->CR &= ~ADC_CR_ADCALDIF;
    ADC1->CR |=  ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL);

    ADC1->CFGR  = 0;
    ADC1->SMPR1 = 0x3FFFFFFF;
    ADC1->SMPR2 = 0x3FFFFFFF;
    ADC1->SQR1  = 0;

    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY));
}

//  DAC (bare register)
static void dac_init(void) {
    RCC->APB1ENR |= RCC_APB1ENR_DAC1EN;
    RCC->AHBENR  |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER |=  (0x3 << (4 * 2));
    GPIOA->PUPDR &= ~(0x3 << (4 * 2));
    DAC->CR  = 0;
    DAC->CR |= DAC_CR_EN1;
    DAC->CR |= DAC_CR_BOFF1;
}

static inline void dac_write(uint16_t val) {
    DAC->DHR12R1 = val & 0x0FFF;
}

//  ISR
void audioISR() {

    // Knobs
    knobDivider++;
    if (knobDivider >= 256) {
        knobDivider = 0;
        uint16_t k1 = analogRead(knob1);
        crushFactor = (int)((k1 / 4095.0f) * 19.0f) + 1;  // 1~20

        uint16_t k2 = analogRead(knob2);
        pitchRate = (k2 / 4095.0f) * 1.0f + 0.5f;         // 0.5~1.5
    }

    // audio in via bare register ADC1_IN1
    int input = adc_read_ch(1);

    // bitcrush
    holdCounter++;
    if (holdCounter >= crushFactor) {
        holdCounter = 0;
        lastSample  = input;
    }

    // pitch shift ring buffer
    pitchBuffer[writeIndex] = (int16_t)lastSample;
    writeIndex = (writeIndex + 1) % BUFFER_SIZE;

    readIndex += pitchRate;
    if (readIndex >= BUFFER_SIZE) readIndex -= BUFFER_SIZE;
    if (readIndex <  0.0f)        readIndex += BUFFER_SIZE;

    // linear interpolation
    int   idx0 = (int)readIndex % BUFFER_SIZE;
    int   idx1 = (idx0 + 1) % BUFFER_SIZE;
    float frac = readIndex - (int)readIndex;
    int   out  = (int)(pitchBuffer[idx0] * (1.0f - frac) + pitchBuffer[idx1] * frac);

    dac_write((uint16_t)constrain(out, 0, 4095));
}

void setup() {
    analogReadResolution(12);

    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    RCC->AHBENR |= RCC_AHBENR_ADC12EN;

    // PA0 analog mode for audio in
    GPIOA->MODER |=  (0x3 << (0 * 2));
    GPIOA->PUPDR &= ~(0x3 << (0 * 2));
    // PA4 configured inside dac_init()

    ADC12_COMMON->CCR = ADC12_CCR_CKMODE_0;
    adc_init();
    dac_init();

    audioTimer = new HardwareTimer(TIM2);
    audioTimer->setOverflow(SAMPLE_RATE, HERTZ_FORMAT);
    audioTimer->attachInterrupt(audioISR);
    audioTimer->resume();
}

void loop() {}