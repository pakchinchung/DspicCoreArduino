/*
 * AkSpiLoopback - dspicArduino (dsPIC33AK128MC106)
 *
 * Internal SPI loopback on AK: SDI is PPS-mapped to the SAME pin SDO drives (RD5),
 * so each SPI.transfer() reads back the byte it sent — no external wiring. The
 * PASS/FAIL prints on Serial (UART1 -> the MCP2221A COM @ 9600).
 * Build with Tools > Clock = "100 MIPS".
 */
#include <SPI.h>

void setup() {
    Serial.begin(9600);
    SPI.setPins(RD6, RD5, RD5);   // SCK=RD6, SDO=RD5, SDI=RD5 (internal loopback)
    SPI.begin();
    Serial.println("=== AK SPI loopback test ===");
}

void loop() {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    const uint8_t tv[] = { 0x00, 0xFF, 0xA5, 0x5A, 0x01, 0x80, 0x42, 0x3C };
    int pass = 0, total = 0;
    for (uint8_t i = 0; i < sizeof(tv); i++) {
        uint8_t r = SPI.transfer(tv[i]);
        total++;
        if (r == tv[i]) pass++;
    }
    SPI.endTransaction();

    Serial.print("SPI loopback: ");
    Serial.print(pass); Serial.print("/"); Serial.print(total);
    Serial.println(pass == total ? "  PASS" : "  FAIL");
    delay(1000);
}
