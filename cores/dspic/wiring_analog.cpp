// analogRead(), analogWrite()
// TODO Phase 5: implement ADC1 for analogRead, Output Compare for analogWrite.

#include "Arduino.h"
#include <xc.h>

int analogRead(uint8_t pin)
{
    // TODO: configure ADC1, set channel from g_PinMap[pin].adcChannel,
    // trigger conversion, wait for ADSTAT, return 10-bit result.
    (void)pin;
    return 0;
}

void analogWrite(uint8_t pin, int val)
{
    // TODO: configure Output Compare module mapped via PPS to pin,
    // set PWM duty cycle proportional to val (0-255).
    (void)pin;
    (void)val;
}
