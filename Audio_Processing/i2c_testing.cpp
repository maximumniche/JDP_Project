#include <Wire.h>

void setup() {
    Wire.begin(); // master mode
    Serial.begin(115200);
}

void loop() {
    Wire.beginTransmission(0x68); // device address
    Wire.write(0x00);             // register you want to read
    Wire.endTransmission();

    Wire.requestFrom(0x68, 1);    // request 1 byte
    if (Wire.available()) {
        byte data = Wire.read();
        Serial.println(data);
    }
}