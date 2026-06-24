/*
 * I2cScanner - dspicArduino
 *
 * Scans the I2C bus on Wire (= I2C1) and prints every 7-bit address that ACKs.
 * On this board I2C1 defaults to its ALTERNATE pins: RC8 = SDA, RC9 = SCL
 * (add ~4.7k pull-ups to 3.3V on each line). A 1602 LCD's PCF8574 backpack
 * usually answers at 0x27 or 0x3F.
 *
 * Use Wire1 (I2C2) or Wire2 (I2C3) the same way for the other buses.
 */
#include <Wire.h>

void setup() {
  Serial.begin(9600);
  Wire.begin();                 // I2C1, RC8/RC9
  Serial.println("I2C scanner ready");
}

void loop() {
  uint8_t count = 0;
  for (uint8_t addr = 0x03; addr <= 0x77; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("  device at 0x");
      Serial.println(addr, HEX);
      count++;
    }
  }
  if (count == 0) Serial.println("  no devices (check wiring + pull-ups)");
  else { Serial.print("  total devices: "); Serial.println(count); }
  Serial.println();
  delay(3000);
}
