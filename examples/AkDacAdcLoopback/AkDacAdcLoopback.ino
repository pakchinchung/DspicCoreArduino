/*
 * AkDacAdcLoopback - dspicArduino (dsPIC33AK128MC106)
 *
 * Verifies DAC + ADC together with NO external wiring: on this device DACOUT1
 * and ADC channel AD1AN7 are the SAME pad (pin RA1, PGC2/DACOUT1/AD1AN7/...),
 * so DAC1 drives RA1 and the ADC samples it on the same pin. The ADC reading
 * should track the DAC value. Output on Serial (UART1 -> MCP2221A COM) @ 9600.
 *
 * Build with Tools > Clock = "100 MIPS".
 */
#define ADC_CHANNEL 7          // AD1AN7 == RA1 == DACOUT1 (internal same-pin loopback)

void setup() {
    Serial.begin(9600);
    analogReadResolution(12);  // compare on the same 12-bit scale as the DAC
    delay(50);
    Serial.println("=== AK DAC -> ADC loopback (RA1, expect ADC ~= DAC) ===");
}

void loop() {
    const uint16_t steps[] = { 0, 512, 1024, 2048, 3072, 4095 };
    int ok = 0;
    for (uint8_t i = 0; i < 6; i++) {
        dacWrite(0, steps[i]);
        delay(5);                                  // let it settle
        int adc = analogRead(ADC_CHANNEL);
        long err = (long)adc - (long)steps[i];
        if (err < 0) err = -err;
        bool near = (err < 250);                   // ~6% of full scale
        if (near) ok++;
        Serial.print("DAC="); Serial.print(steps[i]);
        Serial.print("  ADC="); Serial.print(adc);
        Serial.println(near ? "  ok" : "  (off)");
    }
    Serial.print("tracking: "); Serial.print(ok); Serial.println("/6");
    Serial.println();
    delay(3000);
}
