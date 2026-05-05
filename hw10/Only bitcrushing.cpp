
#include <Arduino.h>
#include <HardwareTimer.h>

// A0 = PA0 → ADC1_IN1  (audio in)
// A1 = PA1 → ADC1_IN2  (knob 1 for bitcrush)
// A2 = PA4 → DAC1_OUT1 (audio out)

#define SAMPLE_RATE 44100

HardwareTimer *audioTimer;

int   crushFactor = 1;   // 1~20 by knob1 
int   holdCounter = 0;
int   lastSample  = 0;
int   knobDivider = 0;

//  ADC
static uint16_t adc_read(ADC_TypeDef *adc, uint8_t ch) {
    adc->SQR1 = (adc->SQR1 & ~ADC_SQR1_SQ1_Msk) | ((ch & 0x1F) << ADC_SQR1_SQ1_Pos);
    adc->CR  |= ADC_CR_ADSTART;
    while (!(adc->ISR & ADC_ISR_EOC));
    return (uint16_t)(adc->DR & 0x0FFF);
}

static void adc_init(ADC_TypeDef *adc) {
    if (adc->CR & ADC_CR_ADEN) {
        adc->CR |= ADC_CR_ADDIS;
        while (adc->CR & ADC_CR_ADEN);
    }
    adc->ISR = 0xFFFFFFFF;

    adc->CR &= ~ADC_CR_ADVREGEN_Msk;
    adc->CR |=  ADC_CR_ADVREGEN_0;
    delayMicroseconds(20);

    adc->CR &= ~ADC_CR_ADCALDIF;
    adc->CR |=  ADC_CR_ADCAL;
    while (adc->CR & ADC_CR_ADCAL);

    adc->CFGR  = 0;
    adc->SMPR1 = 0x3FFFFFFF;
    adc->SMPR2 = 0x3FFFFFFF;
    adc->SQR1  = 0;

    adc->CR |= ADC_CR_ADEN;
    while (!(adc->ISR & ADC_ISR_ADRDY));
}

//  DAC
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
    knobDivider++;
    if (knobDivider >= 256) {
        knobDivider = 0;
        uint16_t k1 = adc_read(ADC1, 2);              // PA1 = ADC1_IN2
        crushFactor = (int)((k1 / 4095.0f) * 19.0f) + 1;  // 1~20
    }

    int input = adc_read(ADC1, 1);   // PA0 = ADC1_IN1

    // Bitcrush
    holdCounter++;
    if (holdCounter >= crushFactor) {
        holdCounter = 0;
        lastSample  = input;
    }

    dac_write((uint16_t)lastSample);
}

//  Setup
void setup() {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;
    RCC->AHBENR |= RCC_AHBENR_ADC12EN;

    // PA0 → Analog
    GPIOA->MODER |=  (0x3 << (0 * 2));
    GPIOA->PUPDR &= ~(0x3 << (0 * 2));

    // PA1 → Analog (knob1)
    GPIOA->MODER |=  (0x3 << (1 * 2));
    GPIOA->PUPDR &= ~(0x3 << (1 * 2));

    // PA4 by dac_init() 

    ADC12_COMMON->CCR = ADC12_CCR_CKMODE_0;
    adc_init(ADC1);
    dac_init();

    audioTimer = new HardwareTimer(TIM2);
    audioTimer->setOverflow(SAMPLE_RATE, HERTZ_FORMAT);
    audioTimer->attachInterrupt(audioISR);
    audioTimer->resume();
}

void loop() {}