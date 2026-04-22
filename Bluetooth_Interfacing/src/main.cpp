#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

AnalogAudioStream out;
BluetoothA2DPSink a2dp_sink(out);

int bluetoothAudio = 25;

void setup() {

    Serial.begin(115200);
    a2dp_sink.start("MyMusic");

    pinMode(bluetoothAudio, OUTPUT);


}

void loop() {

  delay(1000);

}