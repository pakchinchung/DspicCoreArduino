// analogWrite() — dsPIC33CK High-Resolution PWM ("HS PWM") module.
//
// First cut: drives PWM generator PG1 on its PWM1H output pin (a fixed PWM pin,
// not remappable). The duty ratio PG1DC/PG1PER is exact regardless of the PWM
// clock, so LED dimming / analog-average output works; absolute frequency
// calibration is a follow-up. analogWrite(pin, value): pin is currently ignored
// (always PG1); value 0..255 by default (analogWriteResolution to change).

#include "Arduino.h"

#if defined(__dsPIC33C__)
#include "xc_compat.h"

static bool     s_pwm_on = false;
static uint8_t  s_pwm_bits = 8;
#define PWM_PERIOD 20000u                  // PG1PER ticks -> defines the frequency

void analogWriteResolution(uint8_t bits) { s_pwm_bits = bits; }

static void pwmInit()
{
    PCLKCONbits.MCLKSEL = 0;               // PWM master clock source
    PCLKCONbits.DIVSEL  = 0;

    PG1CONLbits.ON     = 0;
    PG1CONLbits.CLKSEL = 1;                // clock from the PWM master clock
    PG1CONLbits.MODSEL = 0;                // independent edge-aligned PWM
    PG1PER   = PWM_PERIOD;
    PG1PHASE = 0;
    PG1DC    = 0;
    PG1IOCONHbits.PMOD = 0;                // complementary/independent output mode
    PG1IOCONHbits.PENH = 1;                // drive the PWM1H pin
    PG1CONLbits.ON     = 1;                // enable the generator
    s_pwm_on = true;
}

void analogWrite(uint8_t pin, int value)
{
    (void)pin;                             // only PG1/PWM1H for now
    if (!s_pwm_on) pwmInit();

    uint16_t maxv = (uint16_t)((1u << s_pwm_bits) - 1);
    if (value < 0) value = 0;
    if (value > (int)maxv) value = maxv;

    // duty = value/maxv * period
    uint32_t dc = ((uint32_t)value * PWM_PERIOD) / maxv;
    PG1DC = (uint16_t)dc;
}

#else  // ---- non-dsPIC33C stub ----
void analogWriteResolution(uint8_t) {}
void analogWrite(uint8_t, int) {}
#endif
