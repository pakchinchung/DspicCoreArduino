/*
 * AkBlink - dspicArduino  (dsPIC33AK128MC106 Curiosity GP DIM board)
 *
 * Blinks the 8 green user LEDs (LED0..LED7 = RC3..RC10) together. This is the
 * AK "first light" sketch: it uses only digital I/O + delay() (no UART/timer
 * interrupts, which aren't ported to AK yet).
 *
 * Timing is APPROXIMATE — the AK clock/PLL isn't configured yet, so the part
 * runs on its power-on clock and the blink rate is uncalibrated (it will blink,
 * just maybe faster/slower than 1 Hz). LED_BUILTIN = LED0 = RC3.
 */
const uint8_t LEDS[] = { LED0, LED1, LED2, LED3, LED4, LED5, LED6, LED7 };

void setup() {
    for (uint8_t i = 0; i < 8; i++) pinMode(LEDS[i], OUTPUT);
}

void loop() {
    for (uint8_t i = 0; i < 8; i++) digitalWrite(LEDS[i], HIGH);
    delay(500);
    for (uint8_t i = 0; i < 8; i++) digitalWrite(LEDS[i], LOW);
    delay(500);
}
