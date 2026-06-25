/*
 * AkI2cScanner - dspicArduino (dsPIC33AK128MC106 Curiosity GP DIM)
 *
 * Scans I2C2 (Wire1) and prints every 7-bit address that ACKs, on Serial
 * (UART1 -> MCP2221A COM @ 9600). The Curiosity LCD is wired to the I2C2
 * ALTERNATE pins (SDA=RD8/ASDA2, SCL=RD7/ASCL2), selected by ALTI2C2=ON in
 * variant.cpp. A PCF8574 LCD backpack typically ACKs at 0x27 or 0x3F.
 * Build with Tools > Clock = "100 MIPS".
 */
#include <Wire.h>

void setup() {
    Serial.begin(9600);
    Wire1.begin();                 // I2C2 (alt pins RD7/RD8)
    delay(50);
    Serial.println("=== AK I2C2 (Wire1) scanner ===");
}

void loop() {
    int n = 0;
    for (uint8_t a = 0x03; a <= 0x77; a++) {
        Wire1.beginTransmission(a);
        if (Wire1.endTransmission() == 0) {
            Serial.print("  device at 0x");
            Serial.println(a, HEX);
            n++;
        }
    }
    Serial.print("scan done, devices = ");
    Serial.println(n);
    delay(2000);
}
