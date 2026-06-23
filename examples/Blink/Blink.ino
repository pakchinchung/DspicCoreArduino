/*
 * Blink - dspicArduino
 *
 * Blinks an LED. The dsPIC pin model is BY NATIVE NAME: set your pin to the
 * datasheet name of the pin your LED is on, e.g. RE6 on the DM330030 board.
 *
 * On DM330030: user LEDs are RE5 and RE6; LED_BUILTIN is RE6.
 * On your own board, just change LED_USER to wherever you wired the LED.
 */

#define LED_USER  RE6      // <-- set this to your LED's pin (e.g. RE5, RA0, ...)

void setup() {
    pinMode(LED_USER, OUTPUT);
}

void loop() {
    digitalWrite(LED_USER, HIGH);
    delay(500);
    digitalWrite(LED_USER, LOW);
    delay(500);
}
