/*
 * AkSerial1Loopback - dspicArduino (dsPIC33AK128MC106)
 *
 * Verifies Serial1 (UART2) on AK via internal PPS loopback: U2RX is mapped to the
 * SAME pin U2TX drives (RD4), so each byte written comes straight back. The
 * PASS/FAIL prints on the console Serial (UART1 -> MCP2221A COM @ 9600).
 * Build with Tools > Clock = "100 MIPS".
 */
const uint8_t TV[] = { 0x00, 0xFF, 0xA5, 0x5A, 0x42, 0x3C, 0x81, 0x18 };

void setup() {
    Serial.begin(9600);            // console on UART1 / COM24
    Serial1.setPins(RD4, RD4);     // U2TX = U2RX = RD4 -> internal loopback
    Serial1.begin(9600);
    delay(50);
    Serial.println("=== AK Serial1 (UART2) loopback ===");
}

void loop() {
    int pass = 0, total = 0;
    for (uint8_t i = 0; i < sizeof(TV); i++) {
        while (Serial1.available()) Serial1.read();
        Serial1.write(TV[i]);
        int r = -1;
        unsigned long t0 = millis();
        while (millis() - t0 < 20) { if (Serial1.available()) { r = Serial1.read(); break; } }
        total++;
        if (r == TV[i]) pass++;
    }
    Serial.print("Serial1 loopback: ");
    Serial.print(pass); Serial.print("/"); Serial.print(total);
    Serial.println(pass == total ? "  PASS" : "  FAIL");
    delay(1000);
}
