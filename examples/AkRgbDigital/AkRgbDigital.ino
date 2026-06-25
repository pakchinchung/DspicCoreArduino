/*
 * AkRgbDigital - diagnostic: drive the RGB LED with plain digitalWrite (no PWM).
 * Cycles R, G, B, all-on, all-off. If the LED lights here, the pins + LED are
 * good and the PWM (analogWrite) issue is the PWM clock. RC2=Red, RD0=Green,
 * RD2=Blue. (Polarity unknown: one of on/off phases will light it.)
 */
#define R  LED_RED      // RC2
#define G  LED_GREEN    // RD0
#define B  LED_BLUE     // RD2

void setup() {
    pinMode(R, OUTPUT);
    pinMode(G, OUTPUT);
    pinMode(B, OUTPUT);
}

static void show(int r, int g, int b) {
    digitalWrite(R, r); digitalWrite(G, g); digitalWrite(B, b);
    delay(600);
}

void loop() {
    show(1,0,0);   // red pin high
    show(0,1,0);   // green pin high
    show(0,0,1);   // blue pin high
    show(1,1,1);   // all high
    show(0,0,0);   // all low
}
