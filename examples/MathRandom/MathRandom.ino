/*
 * MathRandom - dspicArduino
 *
 * Compile-check / demo for the Arduino math helpers: map(), random(),
 * randomSeed() and yield(). Confirms a stock idiom builds and runs:
 *   analogWrite(pin, map(analogRead(a), 0, 4095, 0, 255))
 */

void setup() {
    Serial.begin(9600);
    randomSeed(analogRead(0));     // seed from a (noisy) floating ADC pin
}

void loop() {
    // Scale a 12-bit ADC reading (0..4095) to an 8-bit PWM duty (0..255).
    int duty = map(analogRead(0), 0, 4095, 0, 255);
    analogWrite(0, duty);

    long r1 = random(100);         // 0..99
    long r2 = random(10, 20);      // 10..19

    Serial.print("duty="); Serial.print(duty);
    Serial.print(" r1=");  Serial.print(r1);
    Serial.print(" r2=");  Serial.println(r2);

    yield();
    delay(500);
}
