/*
 * HalSanity - dspicArduino
 *
 * Exercises ADC (analogRead), DAC (dacWrite), PWM (analogWrite) and Wire(begin)
 * and prints over Serial. Purpose: confirm each peripheral initializes and the
 * ADC conversion completes (doesn't hang) on real hardware. Accuracy of ADC/DAC/
 * PWM still needs a physical loopback/scope test.
 */
#include <Wire.h>

void setup() {
    Serial.begin(9600);
    Wire.begin();                 // I2C1 master (SCL1/SDA1)
}

void loop() {
    static int n = 0;
    dacWrite(0, (uint16_t)((n * 256) & 0x0FFF));   // ramp DAC1 output
    analogWrite(0, n & 0xFF);                       // ramp PWM (PG1/PWM1H) duty
    int a0 = analogRead(0);                         // read AN0

    Serial.print("n=");    Serial.print(n);
    Serial.print(" AN0="); Serial.print(a0);
    Serial.println("  (adc+dac+pwm+wire init ok)");

    n++;
    delay(1000);
}
