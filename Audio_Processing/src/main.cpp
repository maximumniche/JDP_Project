/******************************************************************************
* Project: JDP26 DIY DJ Deck
* File: main.cpp
*
* Student Names: Imad Khan, Yuqing Zhai
*
* Course: ECE 304 – Junior Design Project (JDP26)
* Instructor: Prof. Baird Soules
*
* Date: 5/11/2026
*
* Description:
This program sets up all the necessary pins and timers for our DJ deck. In setup(), it begins serial transmission to our serial monitor and to our LCD display, sets up audio ADC channels, our rotary encoder, low-pass filter PWM, LCD display, and I2C keyboard. We also set up an audio timer that allow our audio interrupt service routine to interrupt at a rate of 44.1kHz. In our loop, we update our PWM values for our frequency cutoffs if our rotary encoders moves, displaying the new cutoff frequency and PWM values on our serial monitor. We also call a function to capture the changes in our potentiometer values, and pass them as updates for our lcd update function. We also get call our function to get key presses from our I2C keyboard, for playing sample values.

For all of our source files, visit https://github.com/maximumniche/JDP_Project/tree/main/Audio_Processing

*
* Hardware:
* - Microcontrollers: STM32, ESP-32
* - Key components: LPF PCB, SPDT Switch, Rotary Encoder, 3 potentiometers, switch knife, LCD display, I2C keyboard
*
* Notes:
// A1  bitcrush knob
// A4  pitch shift knob
// A5  FM depth knob
// A0  audio in  
// A2  audio out 
// D9  PWM1, D10 PWM2
// D3  encoder CLK, D4 encoder DT
// D8  Serial1 TX -> LCD

// Most of our functions that actually do stuff are in other files, as we wanted to reduce clutter. They can be found in the github linked above.
******************************************************************************/

#include <Arduino.h>
#include <HardwareTimer.h>
#include "audio.h"
#include "encoder.h"
#include "lpf.h"
#include "lcd.h"
#include "keyboard.h"

// Audio timer to use for interrupts
HardwareTimer *audioTimer;

void setup() {

    Serial.begin(115200);
    Serial1.begin(9600);

    audio_init();
    encoder_init();
    lpf_init();
    lcd_setup();
    keyboard_init();

    Serial.println("Ready");

    audioTimer = new HardwareTimer(TIM2);
    audioTimer->setOverflow(SAMPLE_RATE, HERTZ_FORMAT);
    audioTimer->attachInterrupt(audioISR);
    audioTimer->resume();
}

void loop() {

    // update frequency cutoff if encoder moves
    if (stepDirty) {
        stepDirty = false;
        lpf_update(currentStep);
        Serial.print("Cutoff: ");
        Serial.println(cutoffFreq[currentStep]);

        Serial.print("PWM1: ");
        Serial.println(pwm1value[currentStep]);

        Serial.print("PWM2: ");
        Serial.println(pwm2value[currentStep]);
        Serial.println();

    }

    // Call function for knob changes
    knobChanges();

    // Display knob values based on what they are
    float pitchVal = pitchSpeed;
    float srVal = crushFactor;
    float modVal = fmDepth;

    // Update the LCD with values
    lcd_update(pitchVal, srVal, fmDepth);

    // Get button press data from I2C keyboard
    get_keyboard_data();

    // if (recording) {
    //     recording = false;
    //     recordAudio();
    // }

}
