// A1 is potentiometer 1 for bitcrushing
// A4, is potentiometer 2 for pitch shift
// A5 is potentiometer for frequency modulation
// A0 is input into ADC
// A2 is DAC output
// D9 = PWM1, D10=PWM2, D11=Button/Knob
// D8 is Uart0_tx, what's used for sending the data to the LCD (Serial1)
// Serial = Serial2 is serial monitor uart

#include <Arduino.h>
#include <HardwareTimer.h>
#include <math.h>

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 512

HardwareTimer *audioTimer;

/* -------------------------------------------------------------------------- */
/*                           // Function Definitions                          */
/* -------------------------------------------------------------------------- */
static uint16_t adc_read(ADC_TypeDef *adc, uint8_t ch);
static void adc_init(ADC_TypeDef *adc);
static void dac_init();
void audioISR();

int coordsToPos(int x, int y);
void goToPos(int x, int y);
void writeAtPos(int x, int y, int symbol);
void drawColLine(int column, int symbol);
void drawRowLine(int row, int symbol);
void writeStrAtPos(int x, int y, String data);
float samplePin(int pin, int numSamples);
void setupLCD();

/* -------------------------------------------------------------------------- */
/*                                // Constants                                */
/* -------------------------------------------------------------------------- */

int pitchPin = A4;
int srPin = A1;
int modPin = A5;

/* ------------------------------- //  PWM LPF ------------------------------ */
#define pwm1   D9
#define pwm2   D10
#define BUTTON D11

const int cutoffFreq[16] = {12000, 8721, 6338, 4606,
                           3348, 2433, 1768, 1285,
                           934,  679,  493,  358,
                           261,  189,  138,  100};

const int pwm1value[16] = {1556,1064,737,491,348,266,184,143,
                          110,90,72,61,53,47,43,40};

const int pwm2value[16] = {1064,778,532,368,286,225,163,131,
                          102,86,71,62,54,49,45,42};

int currentStep = 0;
int prevButtonState = HIGH;


//  BUFFER 
int16_t buffer[BUFFER_SIZE];
volatile int writeIndex = 0;
float readIndex = 0.0f;


/* -- // Custom Effects (Pitch shift, sampling rate, frequency modulation) -- */
int   crushFactor = 1;
int   holdCounter = 0;
int16_t crushedSample = 0;

float pitchSpeed = 1.0f;

float fmDepth = 0.0f;
float lfoPhase = 0.0f;

int knobDiv = 0;

//  SETUP 
void setup() {

    Serial.begin(9600);
    Serial1.begin(9600);

    // GPIO + ADC
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_ADC12EN;

    GPIOA->MODER |= (0x3 << (0 * 2)); // A0
    GPIOA->MODER |= (0x3 << (1 * 2)); // A1
    GPIOA->MODER |= (0x3 << (4 * 2)); // A4
    GPIOA->MODER |= (0x3 << (5 * 2)); // A5

    ADC12_COMMON->CCR = ADC12_CCR_CKMODE_0;

    adc_init(ADC1);
    dac_init();

    //  PWM LPF 
    pinMode(BUTTON, INPUT_PULLUP);

    analogWriteResolution(12);
    analogWriteFrequency(17578);

    analogWrite(pwm1, pwm1value[currentStep]);
    analogWrite(pwm2, pwm2value[currentStep]);

    Serial.println("LPF Ready");


    // Setup the LCD
    setupLCD();

    //  audio timer 
    audioTimer = new HardwareTimer(TIM2);
    audioTimer->setOverflow(SAMPLE_RATE, HERTZ_FORMAT);
    audioTimer->attachInterrupt(audioISR);
    audioTimer->resume();

}


//  LOOP (PWM control) 
void loop() {

    int buttonState = digitalRead(BUTTON);

    if (buttonState == LOW && prevButtonState == HIGH) {
        currentStep = (currentStep + 1) % 16;

        analogWrite(pwm1, pwm1value[currentStep]);
        analogWrite(pwm2, pwm2value[currentStep]);

        Serial.print("Cutoff: ");
        Serial.println(cutoffFreq[currentStep]);
    }

    prevButtonState = buttonState;

    audioTimer->pause();
    int pitchValRaw = adc_read(ADC1, 7);   // A4, channel 7
    int sampleRateValRaw = adc_read(ADC1, 2); // A1, channel 2
    int modulationValRaw = adc_read(ADC1, 6); // A5, channel 6
    audioTimer->resume();

    // Divide by smaller value so we can reach max, subtract by offset so we can reach by min because of imperfection of potentiometers
    float pitchVal = (4095.0 - pitchValRaw - 100) * 100 / 3900;
    float sampleRateVal = (4095.0 - sampleRateValRaw) * 100 / 3900;
    float modulationVal = (4095.0 - modulationValRaw) * 100 / 3900;

    pitchVal = constrain(pitchVal, 0.0, 100.0);
    sampleRateVal = constrain(sampleRateVal, 0.0, 100.0);
    modulationVal = constrain(modulationVal, 0.0, 100.0);

    String pitchValStr = String(pitchVal, 2);
    String sampleRateValStr = String(sampleRateVal, 2);
    String modulationValStr = String(modulationVal, 2);

    // Clamp the size
    pitchValStr = pitchValStr.substring(0, 5);
    sampleRateValStr = sampleRateValStr.substring(0, 5);
    modulationValStr = modulationValStr.substring(0, 5);


    writeStrAtPos(1, 2, pitchValStr);
    writeStrAtPos(7, 2, sampleRateValStr);
    writeStrAtPos(13, 2, modulationValStr);

    delay(50);
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

    GPIOA->MODER |= (0x3 << (4 * 2)); // PA4 DAC
    GPIOA->PUPDR &= ~(0x3 << (4 * 2));

    DAC->CR = DAC_CR_EN1 | DAC_CR_BOFF1;
}

static inline void dac_write(uint16_t v) {
    DAC->DHR12R1 = v & 0x0FFF;
}


//  AUDIO ISR 
void audioISR() {

    //  knobs 
    knobDiv++;
    if (knobDiv >= 256) {
        knobDiv = 0;

        uint16_t k1 = adc_read(ADC1, 2); // bitcrush (A1)
        uint16_t k2 = adc_read(ADC1, 7); // pitch (A4)
        uint16_t k3 = adc_read(ADC1, 6); // FM (A5)

        crushFactor = (k1 * 19) / 4095 + 1;
        pitchSpeed  = 0.5f + (k2 / 4095.0f) * 1.5f;
        fmDepth     = (k3 / 4095.0f) * 0.8f;
    }

    //  input 
    int input = adc_read(ADC1, 1);
    int16_t centered = (int16_t)input - 2048;

    //  bitcrush 
    holdCounter++;
    if (holdCounter >= crushFactor) {
        holdCounter = 0;
        crushedSample = centered;
    }

    //  write buffer 
    buffer[writeIndex] = crushedSample;
    writeIndex = (writeIndex + 1) % BUFFER_SIZE;

    //  FM wobble 
    lfoPhase += (5.0f / SAMPLE_RATE);
    if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;

    float lfo = sinf(lfoPhase * 6.28318f);

    float modSpeed = pitchSpeed + lfo * fmDepth;

    if (modSpeed < 0.2f) modSpeed = 0.2f;
    if (modSpeed > 2.5f) modSpeed = 2.5f;

    //  read buffer 
    readIndex += modSpeed;
    if (readIndex >= BUFFER_SIZE) readIndex -= BUFFER_SIZE;

    int i0 = (int)readIndex;
    int i1 = (i0 + 1) % BUFFER_SIZE;
    float frac = readIndex - i0;

    float out =
        buffer[i0] * (1.0f - frac) +
        buffer[i1] * frac;

    out += 2048.0f;

    if (out > 4095) out = 4095;
    if (out < 0) out = 0;

    dac_write((uint16_t)out);
}

// Map LCD coordinates on display to their location number. Coordinates start at (0,0) top left, end at (20, 4) in bottom right
int coordsToPos(int x, int y) {
    int rowOffsets[] = {0x00, 0x40, 0x14, 0x54};

    int position = rowOffsets[y] + x;

    return position;

}

void goToPos(int x, int y) {
    Serial1.write(0xFE);
    Serial1.write(0x45);
    Serial1.write(coordsToPos(x, y));
}

void writeStrAtPos(int x, int y, String data) {
    goToPos(x, y);
    Serial1.print(data);
}

void writeAtPos(int x, int y, int symbol) {
    goToPos(x, y);
    Serial1.write(symbol);
}

void drawColLine(int column, int symbol) {

    for (int i=0; i < 4; i++) {

        writeAtPos(column, i, symbol);
        
    }

}

void drawRowLine(int row, int symbol) {

    for (int i=0; i < 20; i++) {
        
        writeAtPos(i, row, symbol);
        
    }

}

float samplePin(int pin, int numSamples) {

    float value = 0;

    for (int i=0; i < numSamples; i++) {
        value += analogRead(pin);
    }

    float avgValue = value / numSamples;

    return avgValue;

}

void setupLCD() {

    // Clear LCD
    Serial1.write(0xFE);
    Serial1.write(0x51);

    // Write knob information
    Serial1.write(0xFE);
    Serial1.write(0x45);
    Serial1.write(0x41);
    Serial1.write("Pitch ");
    Serial1.write("SR    ");
    Serial1.write("Mod  ");

    drawRowLine(0, '=');
    drawRowLine(3, '=');

    drawColLine(0, '|');
    drawColLine(6, '|');
    drawColLine(12, '|');
    drawColLine(19, '|');

    writeAtPos(0, 0, 0xAF);
    writeAtPos(19, 3, 0xAF);

}