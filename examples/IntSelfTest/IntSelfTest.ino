/*
 * IntSelfTest - dspicArduino (dsPIC33AK128MC106)
 *
 * Autonomous attachInterrupt() verification with NO external wiring or button.
 * Trick: pin RA2 floats on the Curiosity GP DIM (no board load), so toggling the
 * MCU's internal pull-up/pull-down actually swings the pin's input level -- a
 * REAL input edge the Change Notification hardware detects (unlike a self-driven
 * output, which AK's CN does not flag). We attach a CHANGE interrupt, count ISR
 * fires while swinging the pin, then detachInterrupt() and confirm it stops.
 * Result on Serial (UART1 -> MCP2221A COM) @ 9600.  Build with Clock = 100 MIPS.
 *
 * For the everyday button use-case see PinInterrupt (CK) / AkPinInterrupt (AK).
 */
#include <xc.h>
#define PULL_M (1u << 2)             // RA2 = port A bit 2

volatile unsigned isrCount = 0;
void onEdge() { isrCount++; }

// n high/low cycles via the internal pulls == 2n real input edges on RA2.
static unsigned swing(int n) {
    unsigned before = isrCount;
    for (int i = 0; i < n; i++) {
        CNPDA &= ~PULL_M; CNPUA |=  PULL_M; delay(3);    // pull high
        CNPUA &= ~PULL_M; CNPDA |=  PULL_M; delay(3);    // pull low
    }
    CNPDA &= ~PULL_M; delay(5);
    return isrCount - before;
}

void setup() {
    Serial.begin(9600);
    attachInterrupt(RA2, onEdge, CHANGE);     // RA2 -> digital input + CN edge detect
    delay(50);
    Serial.println("=== AK attachInterrupt self-test (RA2, no wiring) ===");
}

void loop() {
    unsigned edges = swing(10);               // expect 20 (CHANGE = both edges)
    Serial.print("attached: 10 cycles -> "); Serial.print(edges);
    Serial.println(" ISR fires (expect ~20)");

    detachInterrupt(RA2);
    unsigned after = swing(10);
    Serial.print("detached: 10 cycles -> "); Serial.print(after);
    Serial.println(" ISR fires (expect 0)");

    Serial.println((edges >= 18 && after == 0) ? "RESULT: PASS" : "RESULT: CHECK");
    Serial.println();

    attachInterrupt(RA2, onEdge, CHANGE);     // re-arm for next round
    delay(3000);
}
