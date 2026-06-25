/*
 * Eeprom_BootCounter - dspicArduino
 *
 * Self-test for the flash-emulated EEPROM: keeps a boot counter at EEPROM
 * address 0, incremented once per boot and persisted with EEPROM.commit().
 *
 * To prove persistence WITHOUT a reflash (a reflash bulk-erases the EEPROM page),
 * the sketch software-resets itself a few times: you should see the count climb
 * 1, 2, 3, ... on Serial across resets. After a few rounds it stops and idles.
 *
 * Verbose prints (banner / raw read / commit status) make it clear where it gets
 * to if anything stalls. A loop heartbeat confirms it's still alive.
 */
#include <EEPROM.h>

const uint32_t ROUNDS = 6;
uint32_t g_count = 0;

void setup() {
    Serial.begin(9600);
    delay(50);
    Serial.println();
    Serial.println("=== EEPROM boot test ===");

    uint32_t count;
    EEPROM.get(0, count);
    Serial.print("raw read = 0x"); Serial.println(count, HEX);
    if (count == 0xFFFFFFFFUL) count = 0;     // blank/erased Flash reads 0xFF..

    count++;
    EEPROM.put(0, count);

    Serial.println("committing...");
    bool ok = EEPROM.commit();
    Serial.print("commit = "); Serial.println(ok ? "OK" : "FAIL");

    g_count = count;
    Serial.print("boot count = "); Serial.println(count);

    if (count < ROUNDS) {
        Serial.println("  resetting to test persistence...");
        delay(800);
        __asm__ volatile ("reset");           // software reset; Flash preserved
    } else {
        Serial.println("  done - count survived resets.");
    }
}

void loop() {
    Serial.print("idle, count="); Serial.println(g_count);
    delay(1500);
}
