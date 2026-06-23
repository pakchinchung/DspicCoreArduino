/*
 * SerialEcho - dspicArduino
 *
 * Prints a banner, then echoes back every byte it receives, and prints a
 * counter once a second. On the DM330030, Serial goes to the on-board PKoB4
 * virtual COM port (UART1 on RD4=TX / RD3=RX) — open it at 9600 baud.
 */

unsigned long last = 0;
unsigned int  n = 0;

void setup() {
    Serial.begin(9600);
    Serial.println("dspicArduino UART ready");
}

void loop() {
    // Echo whatever arrives.
    while (Serial.available()) {
        char c = Serial.read();
        Serial.write(c);
    }
    // Heartbeat once a second.
    if (millis() - last >= 1000) {
        last = millis();
        Serial.print("tick ");
        Serial.println(n++);
    }
}
