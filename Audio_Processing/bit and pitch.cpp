// A1, (ADC1, 2) (knob 1 for bitcrush)
// A4, (ADC1, 7) (knob 2 for pitch shifting)
// A5, (ADC1, 6) (knob 3 for frequency modulation)


#include <Arduino.h>
#include <HardwareTimer.h>

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 1024   

HardwareTimer *audioTimer;

//  audio buffer 
uint16_t buffer[BUFFER_SIZE];
volatile int writeIndex = 0;

// playback state
float readIndex = 0.0f;
float speed = 1.0f;

// bitcrush
int crushFactor = 1;
int holdCounter = 0;
uint16_t crushedSample = 0;

// knobs
int knobDiv = 0;


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

    adc->CR &= ~ADC_CR_ADCALDIF;
    adc->CR |= ADC_CR_ADCAL;
    while (adc->CR & ADC_CR_ADCAL);

    adc->CFGR = 0;
    adc->SMPR1 = 0x3FFFFFFF;
    adc->SMPR2 = 0x3FFFFFFF;
    adc->SQR1 = 0;

    adc->CR |= ADC_CR_ADEN;
    while (!(adc->ISR & ADC_ISR_ADRDY));
}


//  DAC 
static void dac_init() {
    RCC->APB1ENR |= RCC_APB1ENR_DAC1EN;
    RCC->AHBENR  |= RCC_AHBENR_GPIOAEN;

    GPIOA->MODER |= (0x3 << (4 * 2));
    GPIOA->PUPDR &= ~(0x3 << (4 * 2));

    DAC->CR = DAC_CR_EN1 | DAC_CR_BOFF1;
}

static inline void dac_write(uint16_t v) {
    DAC->DHR12R1 = v & 0x0FFF;
}


//  AUDIO ISR 
void audioISR() {

    // -------- knob read --------
    knobDiv++;
    if (knobDiv >= 256) {
        knobDiv = 0;

        uint16_t k1 = adc_read(ADC1, 2); // bitcrush A1
        uint16_t k2 = adc_read(ADC1, 7); // pitch A4

        crushFactor = (k1 * 19) / 4095 + 1;

        // REAL pitch control
        speed = 0.5f + (k2 / 4095.0f) * 1.5f;
    }

    // -------- input sample --------
    uint16_t in = adc_read(ADC1, 1);

    // -------- bitcrush --------
    holdCounter++;
    if (holdCounter >= crushFactor) {
        holdCounter = 0;
        crushedSample = in;
    }

    // -------- write into buffer (recording) --------
    buffer[writeIndex] = crushedSample;
    writeIndex = (writeIndex + 1) % BUFFER_SIZE;

    // -------- playback pointer (THIS IS THE KEY) --------
    readIndex += speed;

    if (readIndex >= BUFFER_SIZE) {
        readIndex -= BUFFER_SIZE;
    }

    // linear interpolation (important for stability)
    int i0 = (int)readIndex;
    int i1 = (i0 + 1) % BUFFER_SIZE;
    float frac = readIndex - i0;

    uint16_t out = (1 - frac) * buffer[i0] + frac * buffer[i1];

    dac_write(out);
}


//  SETUP 
void setup() {

    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_ADC12EN;

    // A0 audio
    GPIOA->MODER |= (0x3 << (0 * 2));

    // A1 crush
    GPIOA->MODER |= (0x3 << (1 * 2));

    // A4 pitch
    GPIOA->MODER |= (0x3 << (4 * 2));

    ADC12_COMMON->CCR = ADC12_CCR_CKMODE_0;

    adc_init(ADC1);
    dac_init();

    audioTimer = new HardwareTimer(TIM2);
    audioTimer->setOverflow(SAMPLE_RATE, HERTZ_FORMAT);
    audioTimer->attachInterrupt(audioISR);
    audioTimer->resume();
}

void loop() {}