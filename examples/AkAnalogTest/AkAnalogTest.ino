/*
 * AkAnalogTest - dspicArduino (dsPIC33AK128MC106 Curiosity GP DIM)
 *
 * Reads the 10k potentiometer and streams the value over Serial (COM, 9600).
 * The pot is on RA7 = AD1AN6, so analogRead(6). Turn the pot and watch the
 * value sweep 0..4095 (12-bit). Build with Tools > Clock = "100 MIPS".
 */
#define POT_AN  6        // AD1AN6 (RA7) = Curiosity potentiometer

void setup() {
    Serial.begin(9600);
    analogReadResolution(12);          // 0..4095 (default is 10-bit, 0..1023)
    Serial.println("=== AK analog (pot) test ===");
}

void loop() {
    int v = analogRead(POT_AN);
    Serial.print("pot="); Serial.println(v);
    delay(250);
}
