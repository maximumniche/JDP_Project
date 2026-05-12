#include "audio.h"
#include "keyboard.h"
#include "samples.h"
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
volatile int useAnalog = 1;

const int recordingSampleRate = 11025;
const int recordingLength = 30000;
int16_t recordingAudio[recordingLength];

// Bluetooth I2S audio
// I2S2_CK = PB13, I2S2_WS = PB12, I2S2ext_SD = PB14, I2S2SD = PB15
// I2S Init
static void i2s_init(void) {
    RCC->AHBENR  |= RCC_AHBENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

    // PB12 = WS — AF5
    GPIOB->MODER   |=  (2 << (12*2));
    GPIOB->OSPEEDR |=  (1 << (12*2));
    GPIOB->PUPDR   &= ~(3 << (12*2));
    GPIOB->AFR[1]  |=  (5 << ((12-8)*4));

    // PB13 = CK — AF5
    GPIOB->MODER   |=  (2 << (13*2));
    GPIOB->OSPEEDR |=  (1 << (13*2));
    GPIOB->PUPDR   &= ~(3 << (13*2));
    GPIOB->AFR[1]  |=  (5 << ((13-8)*4));

    // PB14 = I2S2ext_SD (MISO/RX) — AF5
    GPIOB->MODER   |=  (2 << (14*2));
    GPIOB->OSPEEDR |=  (1 << (14*2));
    GPIOB->PUPDR   &= ~(3 << (14*2));
    GPIOB->AFR[1]  |=  (5 << ((14-8)*4));

    // SPI2 as I2S slave TX (must be enabled to use I2S2ext RX)
    SPI2->I2SCFGR = SPI_I2SCFGR_I2SMOD |
                    SPI_I2SCFGR_I2SCFG_0;  // 0b01 = slave transmit
    SPI2->I2SPR   = 0;
    SPI2->I2SCFGR |= SPI_I2SCFGR_I2SE;

    // I2S2ext slave receive on PB14
    I2S2ext->I2SCFGR = SPI_I2SCFGR_I2SMOD |
                       SPI_I2SCFGR_I2SCFG_0;  // 0b01 = slave receive
    I2S2ext->I2SPR   = 0;
    I2S2ext->I2SCFGR |= SPI_I2SCFGR_I2SE;
}

// Read I2S data from a left stereo
static uint16_t i2s_read(void) {

    uint32_t timeout = 10000;
    while (!(I2S2ext->SR & SPI_SR_RXNE) && --timeout);
    if (!timeout) return 2048;
    int16_t left = (int16_t)I2S2ext->DR;
    timeout = 10000;
    while (!(I2S2ext->SR & SPI_SR_RXNE) && --timeout);
    if (!timeout) return 2048;
    (void)I2S2ext->DR;
    return (uint16_t)((left + 32768) >> 4);
    
}

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
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN | RCC_AHBENR_ADC34EN;

    // analog pins
    GPIOA->MODER |= (0x3 << (0 * 2));  // A0  audio in
    GPIOA->MODER |= (0x3 << (1 * 2));  // A1  bitcrush knob
    GPIOA->MODER |= (0x3 << (4 * 2));  // A4  pitch knob
    GPIOA->MODER |= (0x3 << (5 * 2));  // A5  FM knob
    GPIOB->MODER |= (0x3 << (0 * 2));  // PB0/A3 mic in

    ADC12_COMMON->CCR = ADC12_CCR_CKMODE_0;
    ADC34_COMMON->CCR = ADC34_CCR_CKMODE_0;

    adc_init(ADC1);
    adc_init(ADC3);
    dac_init();
    i2s_init();
    delay(100);  // let I2S2ext lock onto the incoming clock before audio starts

}

// Knob modifications
void knobChanges() {
    noInterrupts();
    k1 = adc_read(ADC1, 2);  // bitcrush  A1 ch2
    k2 = adc_read(ADC1, 7);  // pitch     A4 ch7
    k3 = adc_read(ADC1, 6);  // FM        A5 ch6


    crushFactor = (k1 * 19) / 4095 + 1;
    pitchSpeed  = 0.5f + (k2 / 4095.0f) * 1.5f;
    fmDepth = (k3 / 4095.0f) * 0.8f;

    useAnalog = digitalRead(PB7);

    // Anchor mod for normality
    if (fmDepth < 0.08f) {
        fmDepth = 0.0f;
    }

    // Anchor pitch for normality
    if (pitchSpeed > 0.90 && pitchSpeed < 1.10) {
        pitchSpeed = 1.0f;
    }

    interrupts();
}

void recordAudio() {
    noInterrupts();  // stop ISR
    
    for (int i = 0; i < recordingLength; i++) {
        recordingAudio[i] = adc_read(ADC3, 12);
        delayMicroseconds(1000000 / recordingSampleRate);  // maintain sample rate
    }
    
    interrupts();  // restart ISR
}

// ISR
void audioISR() {

    /* --------------------------- // Live audio mods --------------------------- */
    // Analog input (ch1 = PA0 = A0)
    // int input = adc_read(ADC1, 1);
    // int16_t centered = (int16_t)input - 2048;
    

    // Switch between I2S bluetooth input and analog input
    int input = useAnalog ? adc_read(ADC1, 1) : i2s_read();

    // dac_write(input);

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
    lfo = lfo * lfo * lfo;
    // float lfo = (lfoPhase < 0.5f) ? (lfoPhase * 4.0f - 1.0f) : (3.0f - lfoPhase * 4.0f);

    float currentFM = lfo * fmDepth;

    // pitch + FM modulation
    float modSpeed = pitchSpeed + lfo * fmDepth;
    if (modSpeed < 0.2f) modSpeed = 0.2f;
    if (modSpeed > 2.5f) modSpeed = 2.5f;
    if (modSpeed < 0.05f) modSpeed = 0.0f;

    // read ring buffer with interpolation
    readIndex += modSpeed;
    if (readIndex >= BUFFER_SIZE) readIndex -= BUFFER_SIZE;

    int   i0   = (int)readIndex;
    int   i1   = (i0 + 1) % BUFFER_SIZE;
    float frac = readIndex - i0;

    float liveOut = buffer[i0] * (1.0f - frac) + buffer[i1] * frac;
    liveOut -= 2048;
    // out += 2048.0f;

    /* ----------------------------- // Sample audio ---------------------------- */

    // --- sample playback path ---
    int32_t sampleOut = 0;
    if (samplePlaying) {
        sampleOut = (audioSamples[sampleNum][sampleIndex] >> 4);
        sampleIndex++;
        if (sampleIndex >= sampleLengths[sampleNum]) {
            sampleIndex = 0;
            samplePlaying = false;
        }
    }


    /* ----------------------------- // Playing recorded audio ---------------------------- */

    // --- Recording playback path ---
    int32_t recordOut = 0;
    if (playing) {

        recordOut = recordingAudio[playbackIndex] - 2048;
        playbackIndex++;
        if (playbackIndex >= recordingLength) {
            playbackIndex = 0;
            playing = false;
        }
    }

    // --- mix ---
    int32_t mixed = ((int32_t)(liveOut) + sampleOut) + 2048;
    if (mixed > 4095) mixed = 4095;
    if (mixed < 0)    mixed = 0;
    dac_write((uint16_t)mixed);

}