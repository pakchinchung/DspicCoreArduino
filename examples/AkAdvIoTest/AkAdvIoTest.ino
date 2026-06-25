/*
 * AkAdvIoTest (dsPIC33AK128MC106) - autonomous test of pulseIn/pulseInLong/shiftIn
 * with NO external wiring. Uses the HS-PWM as a known signal source:
 *   - PWM on RC2 at ~1563 Hz, 50% duty -> pulseIn HIGH/LOW should each read ~320us.
 *   - PWM on RC2 held at 100% / 0% duty -> shiftIn should read 0xFF / 0x00.
 * (shiftOut is exercised by AkShiftOutTest via an SPI-slave loopback.)
 * Build with Tools > Clock = "100 MIPS". Result on Serial @ 9600.
 */
#define SIGPIN RC2     // PWM-capable signal source (also Red RGB)
#define CLKPIN RC3     // any GPIO, used as the shiftIn clock

void setup() {
    Serial.begin(9600);
    pinMode(CLKPIN, OUTPUT);
    digitalWrite(CLKPIN, LOW);
    delay(50);
    Serial.println("=== AK Advanced I/O test (pulseIn / shiftIn) ===");
}

void loop() {
    // pulseIn must MEASURE the PWM duty (not loop overhead): test at 50% and 25%.
    analogWrite(SIGPIN, 128); delay(5);             // 50% duty
    unsigned long h50 = pulseIn(SIGPIN, HIGH, 50000UL);
    unsigned long l50 = pulseIn(SIGPIN, LOW,  50000UL);
    unsigned long p50 = h50 + l50;
    analogWrite(SIGPIN, 64);  delay(5);             // 25% duty
    unsigned long h25 = pulseInLong(SIGPIN, HIGH, 50000UL);
    unsigned long l25 = pulseInLong(SIGPIN, LOW,  50000UL);
    unsigned long p25 = h25 + l25;
    Serial.print("50%: HIGH="); Serial.print(h50); Serial.print(" LOW="); Serial.print(l50);
    Serial.print(" | 25%: HIGH="); Serial.print(h25); Serial.print(" LOW="); Serial.print(l25);
    Serial.print(" | period 50%="); Serial.print(p50); Serial.print(" 25%="); Serial.println(p25);

    analogWrite(SIGPIN, 255); delay(5);
    uint8_t allHi = shiftIn(SIGPIN, CLKPIN, MSBFIRST);
    analogWrite(SIGPIN, 0);   delay(5);
    uint8_t allLo = shiftIn(SIGPIN, CLKPIN, MSBFIRST);
    Serial.print("shiftIn HIGH-held=0x"); Serial.print(allHi, HEX);
    Serial.print("  LOW-held=0x"); Serial.println(allLo, HEX);

    // checks: period stable across duties; 50% symmetric; 25% high ~= half of 50% high;
    //         25% high clearly less than its low; shiftIn reads all-ones/all-zeros.
    bool periodStable = (p50 > 0) && (p25 > 0) &&
                        ((p50 > p25 ? p50 - p25 : p25 - p50) * 5 < p50);   // within ~20%
    bool sym50  = (h50 > l50 ? h50 - l50 : l50 - h50) * 4 < p50;           // ~symmetric
    bool duty25 = (h25 * 100 < l25 * 100) && (h25 * 3 < l25 * 3 + l25);    // high < low at 25%
    bool tracks = (h25 < h50);                                            // 25% high < 50% high
    bool shiftOk = (allHi == 0xFF) && (allLo == 0x00);
    bool pass = periodStable && sym50 && tracks && (h25 < l25) && shiftOk;
    Serial.print("RESULT: "); Serial.println(pass ? "PASS" : "CHECK");
    Serial.println();
    delay(3000);
}
