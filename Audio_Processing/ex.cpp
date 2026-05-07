#include <Arduino.h>

#define SAMPLE_RATE 44100

//  SETUP 
void setup() {

    Serial.begin(9600);


}

//  LOOP (PWM control) 
void loop() {

    Serial.write("Hello");



}