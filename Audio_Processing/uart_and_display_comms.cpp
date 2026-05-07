#include <Arduino.h>

int coordsToPos(int x, int y);
void goToPos(int x, int y);
void writeAtPos(int x, int y, int symbol);
void drawColLine(int column, int symbol);
void drawRowLine(int row, int symbol);
void writeStrAtPos(int x, int y, String data);
float samplePin(int pin, int numSamples);
void setupLCD();

int pitchPin = A4;
int srPin = A1;
int modPin = A5;

void setup() {

    Serial.begin(9600);
    Serial1.begin(9600);
    analogReadResolution(12);

    setupLCD();


}

void loop() {

    int pitchValRaw = samplePin(pitchPin, 500);
    int sampleRateValRaw = samplePin(srPin, 500);
    int modulationValRaw = samplePin(modPin, 500);

    float pitchVal = (4095.0 - pitchValRaw) * 100 / 4095;
    float sampleRateVal = (4095.0 - sampleRateValRaw) * 100 / 4095;
    float modulationVal = (4095.0 - modulationValRaw) * 100 / 4095;

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

    // Serial.println(analogRead(pitchPin));
    // Serial.println(analogRead(srPin));
    // Serial.println(analogRead(modPin));


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