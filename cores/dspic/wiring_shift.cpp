/*
 * shiftOut() / shiftIn() - dspicArduino.
 *
 * Family-agnostic software (bit-bang) shift, built on digitalWrite/digitalRead —
 * identical behaviour to the stock Arduino implementation. The caller sets the
 * data and clock pins to OUTPUT (shiftOut) / data INPUT + clock OUTPUT (shiftIn)
 * via pinMode() first, exactly as on AVR.
 */
#include "Arduino.h"

void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val)
{
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t bit = (bitOrder == LSBFIRST) ? (val & (1u << i))
                                             : (val & (1u << (7 - i)));
        digitalWrite(dataPin, bit ? HIGH : LOW);
        digitalWrite(clockPin, HIGH);
        digitalWrite(clockPin, LOW);
    }
}

uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder)
{
    uint8_t value = 0;
    for (uint8_t i = 0; i < 8; i++) {
        digitalWrite(clockPin, HIGH);
        uint8_t bit = (digitalRead(dataPin) == HIGH) ? 1 : 0;
        if (bitOrder == LSBFIRST) value |= (uint8_t)(bit << i);
        else                      value |= (uint8_t)(bit << (7 - i));
        digitalWrite(clockPin, LOW);
    }
    return value;
}
