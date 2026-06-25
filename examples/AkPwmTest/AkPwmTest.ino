/*
 * AkPwmTest - dspicArduino (dsPIC33AK128MC106 Curiosity GP DIM)
 *
 * analogWrite() / PWM bring-up on the RGB LED (common-cathode):
 *   Red = RC2 (PWM4H), Green = RD0 (PWM2H), Blue = RD2 (PWM1H).
 * Fades each color up then down, one at a time, announcing each on Serial @ 9600
 * so you can confirm BOTH that it fades smoothly AND that the color matches.
 * Build with Tools > Clock = "100 MIPS".
 */
struct Ch { uint8_t pin; const char* name; };
const Ch CH[3] = { {RC2, "RED"}, {RD0, "GREEN"}, {RD2, "BLUE"} };

void allOff() { analogWrite(RC2, 0); analogWrite(RD0, 0); analogWrite(RD2, 0); }

void setup() {
    Serial.begin(9600);
    allOff();
    delay(50);
    Serial.println("=== AK PWM RGB fade test ===");
}

void loop() {
    for (int c = 0; c < 3; c++) {
        allOff();
        Serial.print("fading "); Serial.println(CH[c].name);
        for (int v = 0;   v <= 255; v += 5) { analogWrite(CH[c].pin, v); delay(12); }
        for (int v = 255; v >= 0;   v -= 5) { analogWrite(CH[c].pin, v); delay(12); }
        analogWrite(CH[c].pin, 0);
        delay(300);
    }
}
