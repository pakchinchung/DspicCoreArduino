/*
 * AkI2cScanner - dspicArduino (dsPIC33AK128MC106 Curiosity GP DIM)
 *
 * Scans I2C2 (Wire1) for devices, then drives a 16x2 I2C LCD on that same bus.
 * The LCD is on the I2C2 ALTERNATE pins (SDA=RD8/ASDA2, SCL=RD7/ASCL2), selected
 * by ALTI2C2=ON in variant.cpp. Because the LCD is on Wire1 (not the default
 * Wire=I2C1), we pass &Wire1 to the LiquidCrystal_I2C constructor. Serial @ 9600.
 * Build with Tools > Clock = "100 MIPS".
 *
 * NOTE: the stock LiquidCrystal_I2C is hard-wired to the default `Wire`. The
 * `LiquidCrystal_I2C(addr, &Wire1, cols, rows)` constructor used below needs the
 * bus-selecting version from:
 *   https://github.com/pakchinchung/LiquidCrystal_I2C  (branch feature/select-i2c-bus)
 *   upstream PR: https://github.com/markub3327/LiquidCrystal_I2C/pull/3
 * Until that's merged, install that fork (or copy its .h/.cpp over your library).
 */
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define LCD_ADDR 0x27          // change to 0x3F if your backpack uses that

LiquidCrystal_I2C lcd(LCD_ADDR, &Wire1, 16, 2);   // <-- LCD on Wire1 (I2C2)

void setup() {
    Serial.begin(9600);
    Wire1.begin();                 // I2C2 (alt pins RD7/RD8)
    delay(50);
    Serial.println("=== AK I2C2 (Wire1) scanner + LCD ===");

    int n = 0;
    for (uint8_t a = 0x03; a <= 0x77; a++) {
        Wire1.beginTransmission(a);
        if (Wire1.endTransmission() == 0) {
            Serial.print("  device at 0x"); Serial.println(a, HEX); n++;
        }
    }
    Serial.print("scan done, devices = "); Serial.println(n);

    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("dsPIC + Arduino");
    lcd.setCursor(0, 1);
    lcd.print("I2C LCD OK!");
    Serial.println("LCD init done");
}

void loop() { delay(2000); }
