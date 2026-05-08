#include "keyboard.h"

volatile bool samplePlaying = false;
volatile int sampleIndex = 0;

void keyboard_init() {
    // SDA = PB9
    // SCL = PB8
    Wire.setSDA(PB9);
    Wire.setSCL(PB8);
    Wire.begin();
}

void get_keyboard_data() {

    Wire.requestFrom(KB_ADDR, 1);
    if (Wire.available()) {

        char c = Wire.read();

        if (c == 0x31) {
            Serial.println("1 pressed");
            samplePlaying = true;
            sampleIndex = 0;
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

}