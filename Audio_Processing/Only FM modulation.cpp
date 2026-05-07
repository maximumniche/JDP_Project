
#include <Arduino.h>
#include <HardwareTimer.h>
#include <math.h>

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 512

// ---------------- pins ----------------
// A0 = audio in  (ADC1_IN1)
// A1 = knob1 FM depth (ADC1_IN2)
// A2 = DAC output (PA4)

// ---------------- buffer ----------------
int16_t pitchBuffer[BUFFER_SIZE];
volatile int writeIndex = 0;
float readIndex = 0.0f;

// ---------------- FM ----------------
float fmDepth = 0.0f;
float lfoPhase = 0.0f;

// ---------------- sine LUT ----------------
int8_t sineTable[256];

// ---------------- control ----------------
int knobDivider = 0;

HardwareTimer *audioTimer;


// ================= ADC low-level =================
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


// ================= DAC =================
static void dac_init() {
    RCC->APB1ENR |= RCC_APB1ENR_DAC1EN;
    RCC->AHBENR  |= RCC_AHBENR_GPIOAEN;

    GPIOA->MODER |= (0x3 << (4 * 2)); // PA4 DAC
    GPIOA->PUPDR &= ~(0x3 << (4 * 2));

    DAC->CR = DAC_CR_EN1 | DAC_CR_BOFF1;
}

static inline void dac_write(uint16_t v) {
    DAC->DHR12R1 = v & 0x0FFF;
}


// ================= AUDIO ISR =================
void audioISR() {

    // -------- knob FM depth (A1 = ADC1_IN2) --------
    knobDivider++;
    if (knobDivider >= 256) {
        knobDivider = 0;

        uint16_t k1 = adc_read(ADC1, 6);
        fmDepth = (k1 / 4095.0f) * 0.8f;   // stable range
    }

    // -------- input audio (A0 = ADC1_IN1) --------
    int input = adc_read(ADC1, 1);

    // center signal (important for stability)
    int16_t centered = (int16_t)input - 2048;

    // -------- write buffer --------
    pitchBuffer[writeIndex] = centered;
    writeIndex = (writeIndex + 1) % BUFFER_SIZE;

    // -------- LFO (FM wobble) --------
    lfoPhase += (5.0f / SAMPLE_RATE);   // 5 Hz wobble (IMPORTANT)
    if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;

    int idx = (int)(lfoPhase * 256) & 0xFF;
    float lfo = sineTable[idx] / 127.0f;

    // -------- pitch modulation --------
    float modRate = 1.0f + lfo * fmDepth;

    readIndex += modRate;

    if (readIndex >= BUFFER_SIZE) readIndex -= BUFFER_SIZE;
    if (readIndex < 0) readIndex += BUFFER_SIZE;

    // -------- interpolation --------
    int i0 = (int)readIndex;
    int i1 = (i0 + 1) % BUFFER_SIZE;

    float frac = readIndex - i0;

    float out =
        pitchBuffer[i0] * (1.0f - frac) +
        pitchBuffer[i1] * frac;

    // re-center to DAC range
    out += 2048.0f;

    // clamp
    if (out > 4095) out = 4095;
    if (out < 0) out = 0;

    dac_write((uint16_t)out);
}


// ================= SETUP =================
void setup() {

    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_ADC12EN;

    // A0 audio in
    GPIOA->MODER |= (0x3 << (0 * 2));

    // A1 knob
    GPIOA->MODER |= (0x3 << (1 * 2));

    // DAC PA4
    GPIOA->MODER |= (0x3 << (4 * 2));

    ADC12_COMMON->CCR = ADC12_CCR_CKMODE_0;

    adc_init(ADC1);
    dac_init();

    // sine LUT
    for (int i = 0; i < 256; i++) {
        sineTable[i] = (int8_t)(sinf(2.0f * PI * i / 256.0f) * 127.0f);
    }

    audioTimer = new HardwareTimer(TIM2);
    audioTimer->setOverflow(SAMPLE_RATE, HERTZ_FORMAT);
    audioTimer->attachInterrupt(audioISR);
    audioTimer->resume();
}

void loop() {}