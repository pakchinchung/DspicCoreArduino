/*
 * PotRGB - dspicArduino
 *
 * Reads a potentiometer on RE3 (ADC channel AN23) and sweeps the color of an
 * RGB LED on RE15 (red) / RE14 (green) / RE13 (blue).
 *
 * RE13/14/15 are plain GPIO (no hardware PWM on this device). Everything that
 * needs steady timing — the 256-step software PWM AND the pot sampling + color
 * update — runs in a TIMER INTERRUPT (SCCP CCP2, ~30 kHz). So the color tracks
 * the pot quickly and never flickers, no matter what loop() does. loop() just
 * prints status occasionally.
 *
 * If colors look inverted, your RGB LED is common-anode — set ACTIVE_HIGH 0.
 */
#include <xc.h>                 // CCP2 timer + ISR

#define POT_CH       23         // AN23 = RE3
#define LED_R        RE15
#define LED_G        RE14
#define LED_B        RE13
#define ACTIVE_HIGH  1

static volatile uint8_t rDuty = 0, gDuty = 0, bDuty = 0;
static volatile int     lastPot = 0;

static inline void drive(uint8_t pin, bool on)
{
    digitalWrite(pin, (on == (ACTIVE_HIGH != 0)) ? HIGH : LOW);
}

// ~30 kHz tick: 256-step software PWM; once per cycle (~117 Hz) sample the pot.
extern "C" void __attribute__((interrupt, no_auto_psv)) _CCT2Interrupt(void)
{
    static uint8_t pwm = 0;
    drive(LED_R, pwm < rDuty);
    drive(LED_G, pwm < gDuty);
    drive(LED_B, pwm < bDuty);
    pwm++;

    if (pwm == 0) {                            // once per full PWM cycle
        int p = analogRead(POT_CH);            // sample pot in the ISR
        int h = (int)(((long)p * 768) / 4096); // color-wheel position 0..767
        uint8_t r, g, b;
        if (h < 256)      { r = 255 - h;       g = h;           b = 0; }
        else if (h < 512) { h -= 256; r = 0;   g = 255 - h;     b = h; }
        else              { h -= 512; r = h;   g = 0;           b = 255 - h; }
        rDuty = r; gDuty = g; bDuty = b;
        lastPot = p;
    }
    _CCT2IF = 0;
}

void setup()
{
    Serial.begin(9600);
    analogReadResolution(12);
    analogRead(POT_CH);                        // init the ADC here (not in the ISR)
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);

    CCP2CON1L = 0;
    CCP2CON1Lbits.CCSEL = 0;        // timer mode
    CCP2CON1Lbits.MOD   = 0;
    CCP2CON1Lbits.CLKSEL = 0;       // system clock
    CCP2CON1Lbits.TMRPS = 0;        // 1:1 prescale
    CCP2PRL = 1666;                 // ~30 kHz tick at FCY=50 MHz
    _CCT2IF = 0;
    _CCT2IE = 1;
    CCP2CON1Lbits.CCPON = 1;

    Serial.println("PotRGB: turn the pot to sweep the RGB color");
}

void loop()
{
    Serial.print("pot="); Serial.print(lastPot);
    Serial.print("  RGB="); Serial.print(rDuty); Serial.print(",");
    Serial.print(gDuty); Serial.print(","); Serial.println(bDuty);
    delay(300);
}
