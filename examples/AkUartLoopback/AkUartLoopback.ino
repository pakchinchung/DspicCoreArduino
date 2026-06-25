/*
 * AkUartLoopback - dspicArduino (dsPIC33AK128MC106)
 *
 * Self-contained UART self-test (no host COM needed): maps U1RX to the SAME pin
 * U1TX drives (RP52) for an internal loopback, sends a byte, reads it back, and
 * shows the result on the LEDs:
 *   LED0 (RC3) steady ON  = loopback PASS (UART TX+RX work)
 *   LED1 (RC4) steady ON  = loopback FAIL (wrong/no byte)
 *   LED7 (RC10) toggling  = sketch alive (loop running)
 *
 * Build with Tools > Clock = "100 MIPS" for an exact 100 MHz clock.
 */
void setup() {
    pinMode(LED0, OUTPUT);
    pinMode(LED1, OUTPUT);
    pinMode(LED7, OUTPUT);
    digitalWrite(LED0, LOW);
    digitalWrite(LED1, LOW);

    Serial.setPins(RD3, RD3);   // U1TX = U1RX = RD3 -> internal loopback
    Serial.begin(9600);
}

void loop() {
    while (Serial.available()) Serial.read();    // drain
    Serial.write(0x55);

    int r = -1;
    unsigned long t0 = millis();
    while (millis() - t0 < 20) {                 // wait up to 20 ms for the echo
        if (Serial.available()) { r = Serial.read(); break; }
    }

    bool pass = (r == 0x55);
    digitalWrite(LED0, pass ? HIGH : LOW);
    digitalWrite(LED1, pass ? LOW : HIGH);
    digitalWrite(LED7, digitalRead(LED7) ? LOW : HIGH);   // heartbeat
    delay(300);
}
