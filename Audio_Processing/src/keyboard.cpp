#include "keyboard.h"

volatile bool samplePlaying = false;
volatile int sampleIndex = 0;
volatile int sampleNum = 0;

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

        for (int i=0; i < 10; i++) {
            if (c == (i + '0')) {
                samplePlaying = true;
                sampleIndex = 0;
                sampleNum = i;
            }
        }

    }

}