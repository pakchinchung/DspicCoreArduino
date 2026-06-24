/*
 * LcdHelloWorld - dspicArduino
 *
 * A 16x2 character LCD on an I2C (PCF8574) backpack, driven by the standard
 * Arduino "LiquidCrystal I2C" library (Frank de Brabander). This exercises the
 * Wire (I2C1) driver end-to-end against a real third-party library.
 *
 *   *** WIRING ***
 *     Backpack SDA -> RC8     (I2C1 alternate pins = board default)
 *     Backpack SCL -> RC9
 *     VCC -> 5V (most 1602 backpacks need 5V), GND -> GND
 *     Pull-ups: most backpacks include them; if not, ~4.7k to 3.3V on each line.
 *
 * If the screen stays blank: run the I2cScanner example to find the backpack
 * address (commonly 0x27 or 0x3F), set LCD_ADDR below, and turn the contrast
 * trimmer on the backpack.
 */
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define LCD_ADDR 0x27          // change to 0x3F if your backpack uses that

LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

void setup() {
  Wire.begin();                // I2C1 on RC8 (SDA) / RC9 (SCL)
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("dsPIC + Arduino");
  lcd.setCursor(0, 1);
  lcd.print("I2C LCD OK!");
}

void loop() {
  static unsigned long n = 0;  // live counter proves the bus keeps working
  lcd.setCursor(12, 1);
  lcd.print(n % 1000);
  lcd.print("   ");
  n++;
  delay(500);
}
