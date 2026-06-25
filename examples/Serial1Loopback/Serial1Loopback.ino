/*
 * Serial1Loopback - dspicArduino
 *
 * Internal UART loopback for Serial1 (UART2): U2RX is PPS-mapped to the SAME pin
 * U2TX drives (RD8), so every byte written comes straight back — no external
 * wiring. Results print on Serial (UART1, PKoB4 COM @ 9600).
 *
 * Pins are chosen by native NAME. For real use, call Serial1.setPins(txPin, rxPin)
 * with distinct pins (or Serial1.begin(baud, txPin, rxPin)).
 */
const uint8_t TV[] = { 0x00, 0xFF, 0xA5, 0x5A, 0x42, 0x3C, 0x81, 0x18 };

void setup() {
    Serial.begin(9600);          // console on UART1 / COM6
    Serial1.setPins(RD8, RD8);   // U2TX = U2RX = RD8 -> internal loopback
    Serial1.begin(9600);
    delay(50);
}

void loop() {
    int pass = 0, total = 0;
    for (uint8_t i = 0; i < sizeof(TV); i++) {
        while (Serial1.available()) Serial1.read();     // drain
        Serial1.write(TV[i]);

        int r = -1;
        unsigned long t0 = millis();
        while (millis() - t0 < 20) {                    // wait up to 20 ms for echo
            if (Serial1.available()) { r = Serial1.read(); break; }
        }
        total++;
        if (r == TV[i]) pass++;
    }

    Serial.print("Serial1 loopback: ");
    Serial.print(pass); Serial.print("/"); Serial.print(total);
    Serial.println(pass == total ? "  PASS" : "  FAIL");
    delay(1500);
}
