/*
 * SpiLoopback - dspicArduino
 *
 * Internal SPI loopback: SDI is PPS-mapped to the SAME pin SDO drives (RP69),
 * so each SPI.transfer() reads back the byte it sent — no external wiring.
 * Results print on Serial (PKoB4 COM @ 9600).
 *
 * For real use, call SPI.setPins(SCK_RP, SDO_RP, SDI_RP) with distinct pins.
 */
#include <SPI.h>

void setup() {
    Serial.begin(9600);
    SPI.setPins(70, 69, 69);   // SCK=RP70(RD6), SDO=RP69(RD5), SDI=RP69 (loopback)
    SPI.begin();
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
    delay(1500);
}
