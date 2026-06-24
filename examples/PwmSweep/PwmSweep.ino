/*
 * PwmSweep - dspicArduino
 *
 * Sweeps the PWM duty 0..255..0 on the HS-PWM output PG1 = PWM1H pin = RB14.
 *
 *   *** Put an LED (+ resistor) or a scope on RB14 to GND ***
 *
 * The LED should fade up and down; on a scope you'll see the duty cycle change.
 * Current duty is printed on Serial @ 9600.
 */

void setup() {
    Serial.begin(9600);
    Serial.println("PWM sweep on RB14 (PWM1H) — watch the LED fade");
}

void loop() {
    static int v = 0, dir = 1;
    analogWrite(0, v);                 // pin arg ignored -> PG1/PWM1H (RB14)
    if ((v % 32) == 0) { Serial.print("duty="); Serial.println(v); }
    v += dir;
    if (v >= 255) { v = 255; dir = -1; }
    if (v <= 0)   { v = 0;   dir = 1;  }
    delay(8);
}
