/*
 * AkEepromTest - dspicArduino (dsPIC33AK128MC106)
 *
 * Autonomous flash-EEPROM persistence test: reads a boot counter from EEPROM,
 * prints it, increments + commit()s it, then software-resets. If the EEPROM truly
 * persists to flash, the counter climbs across reboots (visible on Serial @ 9600).
 * Build with Tools > Clock = "100 MIPS".
 */
#include <EEPROM.h>

void setup() {
    Serial.begin(9600);
    delay(50);

    uint8_t c = EEPROM.read(0);          // persisted across reset (0xFF when blank)
    if (c == 0xFF) c = 0;
    Serial.print("boot count = ");
    Serial.println(c);

    EEPROM.write(0, (uint8_t)(c + 1));
    bool ok = EEPROM.commit();
    Serial.print("commit: ");
    Serial.println(ok ? "OK" : "FAIL");

    delay(1200);
    Serial.println("-- software reset --");
    delay(50);
    asm volatile ("reset");              // reboot; next boot should read c+1
}

void loop() { }
