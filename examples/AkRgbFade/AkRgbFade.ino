/*
 * AkRgbFade - dspicArduino (dsPIC33AK128MC106 Curiosity GP DIM)
 *
 * Cross-fades the on-board RGB LED through the color wheel using analogWrite
 * (High-Speed PWM): Red=RC2 (PWM1H), Green=RD0 (PWM2H), Blue=RD2 (PWM3H), routed
 * via PPS. Build with Tools > Clock = "100 MIPS" (PWM needs the PLL clock).
 */
#define R  LED_RED      // RC2
#define G  LED_GREEN    // RD0
#define B  LED_BLUE     // RD2

void setup() {
    analogWriteResolution(8);     // 0..255 per channel
}

static void fade(uint8_t fromPin, uint8_t toPin) {
    for (int i = 0; i < 256; i++) {
        analogWrite(fromPin, 255 - i);
        analogWrite(toPin, i);
        delay(4);
    }
}

void loop() {
    analogWrite(R, 255); analogWrite(G, 0); analogWrite(B, 0);
    fade(R, G);     // red -> green
    fade(G, B);     // green -> blue
    fade(B, R);     // blue -> red
}
