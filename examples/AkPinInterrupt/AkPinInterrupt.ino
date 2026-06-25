/*
 * AkPinInterrupt - dspicArduino (dsPIC33AK128MC106 Curiosity GP DIM)
 *
 * attachInterrupt() demo via Change Notification. Curiosity user button SW1 is on
 * RB5 (external pull-up: idle HIGH, pressed = LOW -> FALLING edge); user LED0 is
 * on RC3. Each press toggles the LED from the ISR and bumps a counter the main
 * loop prints, so it is verifiable both visually and over Serial @ 9600.
 *
 * Build with Tools > Clock = "100 MIPS".
 */
#define BTN  RB5            // SW1 (external pull-up)
#define LED  RC3            // LED0

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
    attachInterrupt(BTN, onPress, FALLING);
    Serial.println("AkPinInterrupt ready - press SW1 (RB5)");
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
