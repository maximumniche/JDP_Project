#include <Arduino.h>
#include <cmath>

const int pwm1 = D9;
const int pwm2 = D10;
const int button = D8;

int frequency = 17578;
int resolution = 4096;

int cutoffFreq[16] = {12000, 8721, 6338, 4606,
  3348, 2433, 1768, 1285 ,934, 679, 493, 358, 261, 
  189, 138, 100};

int pwm1value[16] =  {1556, 1064, 737, 491, 
  348, 266, 184, 143, 110, 90, 72, 61, 53, 
  47, 43, 40};

int pwm2value[16] =  {1064, 778, 532, 368, 
  286, 225, 163, 131, 102, 86, 71, 62, 54, 
  49, 45, 42};

int current = 0; 
int prevButtonState = digitalRead(button);


void setup() {

  Serial.begin(115200);

  /* 12-bit resolution */
  analogWriteResolution(12);

  /* 17.578 kHz PWM frequency (GLOBAL setting) */
  analogWriteFrequency(frequency);

  analogWrite(pwm1, 0);
  analogWrite(pwm2, 0);
  pinMode(button,INPUT_PULLUP);

  // int value = duty * (std::pow(2,12));

  delay(5000);

  analogWrite(pwm1, pwm1value[current]);
  analogWrite(pwm2, pwm2value[current]);
  Serial.println("PWM test start");
  Serial.print("Cutoff Frequency(Hz): ");
  Serial.println(cutoffFreq[current]);
}

void loop() {

  /*
  
  Serial.print("Enter a duty cycle percentage (1-100): ");

  while (Serial.available() == 0) {}

  String input = Serial.readStringUntil('\n');
  input.trim();

  float duty = input.toFloat();
  int value = (duty * resolution) / 100;

  analogWrite(pwm1, value);
  analogWrite(pwm2, value);

  Serial.print("% | Duty cycle % = ");
  Serial.println(duty);
  
  Serial.print("% | PWM value (0-4095) = ");
  Serial.println(value);

  */

  // Serial.print(digitalRead(button));
  
  if (digitalRead(button) == LOW) {
    prevButtonState = LOW;
    current += 1;
    if (current > 15) {
      current = 0;
    }
    Serial.print("Cutoff Frequency(Hz): ");
    Serial.println(cutoffFreq[current]);
    analogWrite(pwm1, pwm1value[current]);
    analogWrite(pwm2, pwm2value[current]);
  }

  delay(100);
}
