/*
 * AkSerialTest - dspicArduino (dsPIC33AK128MC106)
 *
 * Serial self-test for AK: prints a counter + millis() once per second on
 * Serial (UART1 -> PKOB COM: TX=RD10/RP59, RX=RD9/RP58). Read it on the PKOB
 * virtual COM port. Two checks at once:
 *   1) clean text at 9600  -> UART baud (FUART) is right;
 *   2) millis() increments ~1000 per line AND the host sees ~1 s between lines
 *      -> the clock + Timer1 timing is accurate.
 *
 * Use the Tools > Clock = "100 MIPS" build for an exact 100 MHz clock (clean baud).
 */
void setup() {
    Serial.begin(9600);
    delay(50);
    Serial.println();
    Serial.println("=== AK Serial test ===");
}

void loop() {
    static unsigned long n = 0;
    Serial.print("n="); Serial.print(n);
    Serial.print(" millis="); Serial.println(millis());
    n++;
    delay(1000);
}
