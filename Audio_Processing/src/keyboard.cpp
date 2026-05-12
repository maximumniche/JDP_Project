#include "keyboard.h"

volatile bool recording = false;
volatile bool playing = false;
volatile bool samplePlaying = false;
volatile int sampleIndex = 0;
volatile int playbackIndex = 0;
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

        // Check if a sample was played
        for (int i=0; i < 10; i++) {
            if (c == (i + '0')) {
                samplePlaying = true;
                sampleIndex = 0;
                sampleNum = i;
            }
        }

        // Checking for recording + playcback
        if (c == 'r') {

            recording = true;
            playing = false;
            playbackIndex = 0;
            Serial.println("Recording...");

        }

        if (c == 'p') {
            playing = true;
            recording = false;
            playbackIndex = 0;
            Serial.println("Playing...");
        }

        if (c == 's') {

            playing = false;
            recording = false;
            playbackIndex = 0;
            Serial.println("Recording stopped");


        }

    }

}