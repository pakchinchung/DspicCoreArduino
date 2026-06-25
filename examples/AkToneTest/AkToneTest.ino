/*
 * AkToneTest (dsPIC33AK128MC106) - autonomous test of tone()/noTone(), no wiring.
 * tone() drives a square wave on a pin; we measure it back with pulseIn() on the
 * same pin and check the frequency matches, then noTone() and confirm it stops.
 * Build with Tools > Clock = "100 MIPS". Result on Serial @ 9600.
 */
#define TONEPIN RC3            // any GPIO (LED0); measured via pulseIn

void setup() {
    Serial.begin(9600);
    delay(50);
    Serial.println("=== AK tone() test ===");
}

static bool checkTone(unsigned int f) {
    tone(TONEPIN, f);
    delay(20);
    unsigned long hi = pulseIn(TONEPIN, HIGH, 50000UL);
    unsigned long lo = pulseIn(TONEPIN, LOW,  50000UL);
    unsigned long period = hi + lo;
    unsigned long meas = period ? (1000000UL / period) : 0;
    Serial.print("tone "); Serial.print(f); Serial.print("Hz -> measured ");
    Serial.print(meas); Serial.print("Hz (HIGH="); Serial.print(hi);
    Serial.print("us LOW="); Serial.print(lo); Serial.println("us)");
    // within 8%
    unsigned long d = (meas > f) ? meas - f : f - meas;
    return (d * 100 < f * 8UL);
}

void loop() {
    bool ok = true;
    ok &= checkTone(1000);
    ok &= checkTone(2000);
    ok &= checkTone(4000);

    noTone(TONEPIN);
    delay(10);
    unsigned long after = pulseIn(TONEPIN, HIGH, 20000UL);   // expect 0 (silent)
    Serial.print("after noTone: pulseIn="); Serial.print(after); Serial.println("us (expect 0)");

    Serial.print("RESULT: "); Serial.println((ok && after == 0) ? "PASS" : "CHECK");
    Serial.println();
    delay(3000);
}
