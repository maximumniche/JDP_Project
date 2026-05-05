// A1, (ADC1, 2) (knob 1 for bitcrush)
// A4, (ADC1, 7) (knob 2 for pitch shifting)
// A5, (ADC1, 6) (knob 3 for frequency modulation)


#include <Arduino.h>
#include <HardwareTimer.h>
#include <math.h>

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 512

HardwareTimer *audioTimer;

//  audio buffer (for pitch shifting) 
int16_t buffer[BUFFER_SIZE];     // stores incoming audio samples
volatile int writeIndex = 0;     // write position in buffer
float readIndex = 0.0f;          // read position (for pitch control)

//  effect parameters 
int   crushFactor = 1;           // bitcrush resolution control
int   holdCounter = 0;           // holds samples for bitcrush
uint16_t crushedSample = 0;      // last held sample

float pitchSpeed = 1.0f;         // playback speed (pitch shift)

// FM wobble (LFO modulation)
float fmDepth = 0.0f;            // wobble intensity
float lfoPhase = 0.0f;           // LFO phase

int knobDiv = 0;                 // slows down ADC knob reading


//  ADC read (low-level STM32) 
static uint16_t adc_read(ADC_TypeDef *adc, uint8_t ch) {
    adc->SQR1 = (adc->SQR1 & ~ADC_SQR1_SQ1_Msk) |
                ((ch & 0x1F) << ADC_SQR1_SQ1_Pos);

    adc->CR |= ADC_CR_ADSTART;
    while (!(adc->ISR & ADC_ISR_EOC));

    return adc->DR & 0x0FFF;
}


//  ADC init 
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

    adc->CFGR = 0;
    adc->SMPR1 = 0x3FFFFFFF;
    adc->SMPR2 = 0x3FFFFFFF;
    adc->SQR1 = 0;

    adc->CR |= ADC_CR_ADEN;
    while (!(adc->ISR & ADC_ISR_ADRDY));
}


//  DAC init 
static void dac_init() {
    RCC->APB1ENR |= RCC_APB1ENR_DAC1EN;
    RCC->AHBENR  |= RCC_AHBENR_GPIOAEN;

    GPIOA->MODER |= (0x3 << (4 * 2)); // PA4 = DAC output
    GPIOA->PUPDR &= ~(0x3 << (4 * 2));

    DAC->CR = DAC_CR_EN1 | DAC_CR_BOFF1;
}


// write to DAC
static inline void dac_write(uint16_t v) {
    DAC->DHR12R1 = v & 0x0FFF;
}


//  AUDIO INTERRUPT 
void audioISR() {

    //  read knobs slowly 
    knobDiv++;
    if (knobDiv >= 256) {
        knobDiv = 0;

        uint16_t k1 = adc_read(ADC1, 2); // bitcrush knob
        uint16_t k2 = adc_read(ADC1, 7); // pitch knob
        uint16_t k3 = adc_read(ADC1, 6); // wobble knob

        // map knobs to parameters
        crushFactor = (k1 * 19) / 4095 + 1;
        pitchSpeed  = 0.5f + (k2 / 4095.0f) * 1.5f;
        fmDepth     = (k3 / 4095.0f) * 0.8f;
    }

    //  read input audio 
    int input = adc_read(ADC1, 1);

    // center signal around 0
    int16_t centered = (int16_t)input - 2048;

    //  bitcrush effect 
    holdCounter++;
    if (holdCounter >= crushFactor) {
        holdCounter = 0;
        crushedSample = centered;
    }

    // write into buffer (recording stage)
    buffer[writeIndex] = crushedSample;
    writeIndex = (writeIndex + 1) % BUFFER_SIZE;

    //  LFO for wobble (FM) 
    lfoPhase += (5.0f / SAMPLE_RATE); // ~5 Hz modulation
    if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;

    float lfo = sinf(lfoPhase * 6.28318f);

    // modulate playback speed
    float modSpeed = pitchSpeed + lfo * fmDepth;

    // safety limits
    if (modSpeed < 0.2f) modSpeed = 0.2f;
    if (modSpeed > 2.5f) modSpeed = 2.5f;

    //  read from buffer (pitch shifting) 
    readIndex += modSpeed;

    if (readIndex >= BUFFER_SIZE) readIndex -= BUFFER_SIZE;

    int i0 = (int)readIndex;
    int i1 = (i0 + 1) % BUFFER_SIZE;

    float frac = readIndex - i0;

    // linear interpolation
    float out =
        buffer[i0] * (1.0f - frac) +
        buffer[i1] * frac;

    // shift back to DAC range
    out += 2048.0f;

    // clamp output
    if (out > 4095) out = 4095;
    if (out < 0) out = 0;

    dac_write((uint16_t)out);
}


void setup() {

    // enable GPIO + ADC clocks
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_ADC12EN;

    // A0 = audio input
    GPIOA->MODER |= (0x3 << (0 * 2));

    // A1 = bitcrush knob
    GPIOA->MODER |= (0x3 << (1 * 2));

    // A4 = pitch knob
    GPIOA->MODER |= (0x3 << (4 * 2));

    // A5 = FM wobble knob
    GPIOA->MODER |= (0x3 << (5 * 2));

    ADC12_COMMON->CCR = ADC12_CCR_CKMODE_0;

    adc_init(ADC1);
    dac_init();

    // start audio timer interrupt
    audioTimer = new HardwareTimer(TIM2);
    audioTimer->setOverflow(SAMPLE_RATE, HERTZ_FORMAT);
    audioTimer->attachInterrupt(audioISR);
    audioTimer->resume();
}

void loop() {}