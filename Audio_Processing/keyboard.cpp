#include <Arduino.h>
#include <Wire.h>

#define KB_ADDR 0x5F

void setup() {

    Serial.begin(9600);

    // SDA = PB9
    // SCL = PB8
    Wire.setSDA(PB9);
    Wire.setSCL(PB8);
    Wire.begin();
}

void loop() {

    Wire.requestFrom(KB_ADDR, 1);
    if (Wire.available()) {

        char c = Wire.read();

        if (c == 0x31) {
            Serial.println("1 pressed");
            // int input = adc_read(ADC1, 1);
        }
        else if (c == 0x32) {
            Serial.println("2 pressed");
            // int input = adc_read(ADC1, 1);
        }
        else if (c != 0) {
            Serial.print("Key: ");
            Serial.println(c);
            Serial.print("ASCII: ");
            Serial.println((int)c);
        }
    }

    delay(10);
}