/*
 * PinInterrupt - dspicArduino
 *
 * attachInterrupt() demo / self-test using Change Notification.
 * DM330030: user button on RE7 (external pull-up: idle HIGH, pressed = LOW, so a
 * press is a FALLING edge), user LED on RE6. Each press toggles the LED from the
 * ISR and bumps a counter that the main loop prints — so it's verifiable both
 * visually and over Serial (COM6).
 *
 * Note: RE7/RE6 are NOT PPS-remappable, which is exactly why this uses CN, not
 * the INTx external-interrupt pins (see docs/PROJECT_STATUS.md §4).
 */
#define BTN  RE7
#define LED  RE6

volatile unsigned long g_presses = 0;

void onPress() {
    digitalWrite(LED, digitalRead(LED) ? LOW : HIGH);   // toggle
    g_presses++;
}

void setup() {
    Serial.begin(9600);
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);
    pinMode(BTN, INPUT);                                 // board has ext. pull-up
    attachInterrupt(digitalPinToInterrupt(BTN), onPress, FALLING);
    Serial.println("PinInterrupt ready - press the RE7 button");
}

void loop() {
    static unsigned long last = 0;
    unsigned long now = g_presses;
    if (now != last) {
        last = now;
        Serial.print("button presses: ");
        Serial.println(now);
    }
    delay(50);
}
