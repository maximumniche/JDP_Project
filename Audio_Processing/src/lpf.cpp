#include "lpf.h"

const int cutoffFreq[16] = {12000, 8721, 6338, 4606,
                             3348,  2433, 1768, 1285,
                             934,   679,  493,  358,
                             261,   189,  138,  100};

const int pwm1value[16]  = {1556, 1064, 737, 491,
                             348,  266,  184, 143,
                             110,  90,   72,  61,
                             53,   47,   43,  40};

const int pwm2value[16]  = {1064, 778, 532, 368,
                             286,  225, 163, 131,
                             102,  86,  71,  62,
                             54,   49,  45,  42};

void lpf_init() {
    analogWriteResolution(12);
    analogWriteFrequency(17578);   // 17.578 kHz PWM global
    analogWrite(pwm1, pwm1value[0]);
    analogWrite(pwm2, pwm2value[0]);
}

void lpf_update(int step) {
    analogWrite(pwm1, pwm1value[step]);
    analogWrite(pwm2, pwm2value[step]);
}
