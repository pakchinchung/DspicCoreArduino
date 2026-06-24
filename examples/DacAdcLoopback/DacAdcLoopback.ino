/*
 * DacAdcLoopback - dspicArduino
 *
 * Verifies DAC + ADC together with NO external wiring: on this device DACOUT1
 * and ADC channel AN3 are the SAME pin (RA3), so the DAC drives RA3 and the ADC
 * samples it on the same pin. The ADC reading should track the DAC value.
 * Output on Serial @ 9600.
 */

#define ADC_CHANNEL 3          // AN3 == RA3 == DACOUT1 (internal same-pin loopback)

void setup() {
    Serial.begin(9600);
    analogReadResolution(12);  // compare on the same 12-bit scale as the DAC
}

void loop() {
    const uint16_t steps[] = { 0, 512, 1024, 2048, 3072, 4095 };
    Serial.println("--- DAC -> ADC loopback (expect ADC ~= DAC) ---");
    int ok = 0;
    for (uint8_t i = 0; i < 6; i++) {
        dacWrite(0, steps[i]);
        delay(5);                                  // let it settle
        int adc = analogRead(ADC_CHANNEL);
        long err = (long)adc - (long)steps[i];
        if (err < 0) err = -err;
        bool near = (err < 200);                   // ~5% of full scale
        if (near) ok++;
        Serial.print("DAC="); Serial.print(steps[i]);
        Serial.print("  ADC="); Serial.print(adc);
        Serial.println(near ? "  ok" : "  (off)");
    }
    Serial.print("tracking: "); Serial.print(ok); Serial.println("/6");
    Serial.println();
    delay(3000);
}
