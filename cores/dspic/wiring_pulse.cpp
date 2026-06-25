/*
 * pulseIn() / pulseInLong() - dspicArduino.
 *
 * Measures the width (in microseconds) of a HIGH or LOW pulse on a pin. Both use
 * micros() here (which has sub-microsecond resolution off Timer1), so pulseIn and
 * pulseInLong behave identically on this core — accurate without the fragile
 * cycle-counting calibration the AVR core needs. Returns 0 on timeout (default 1 s)
 * or if no pulse starts. The caller sets the pin to INPUT first.
 */
#include "Arduino.h"

unsigned long pulseInLong(uint8_t pin, uint8_t state, unsigned long timeout)
{
    unsigned long startMicros = micros();

    // 1) if a pulse of the wanted state is already in progress, wait it out
    while (digitalRead(pin) == state)
        if (micros() - startMicros >= timeout) return 0;

    // 2) wait for the pulse to begin
    while (digitalRead(pin) != state)
        if (micros() - startMicros >= timeout) return 0;

    // 3) time the pulse until it ends
    unsigned long t0 = micros();
    while (digitalRead(pin) == state)
        if (micros() - startMicros >= timeout) return 0;

    return micros() - t0;
}

unsigned long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout)
{
    return pulseInLong(pin, state, timeout);
}
