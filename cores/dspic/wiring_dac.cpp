// dacWrite() — dsPIC33CK DAC (part of the high-speed comparator/DAC module).
// 12-bit, output on the DAC1 output pin (DACOUT1). Reference = AVDD:
//   Vout ≈ (value / 4096) * AVDD.
//
// dacWrite(channel, value): channel currently selects DAC1 only; value 0..4095.

#include "Arduino.h"

#if defined(__dsPIC33C__)
#include "xc_compat.h"

static bool s_dac_on = false;

void dacWrite(uint8_t channel, uint16_t value)
{
    (void)channel;                         // only DAC1 for now
    if (value > 4095) value = 4095;

    if (!s_dac_on) {
        DACCTRL1Lbits.DACON = 0;
        DACCTRL1Lbits.CLKSEL = 1;          // DAC clock source = FP
        DACCTRL1Lbits.CLKDIV = 0;
        DACCTRL1Lbits.DACON = 1;           // enable the DAC module
        DAC1CONLbits.DACOEN = 1;           // route DAC1 to its output pin
        DAC1CONLbits.DACEN  = 1;           // enable DAC1
        s_dac_on = true;
    }
    DAC1DATH = value;                      // 12-bit output value
}

#elif defined(__dsPIC33A__)
// dacWrite() — dsPIC33AK DAC1 (high-speed comparator/DAC module, 12-bit).
// Output on DACOUT1, which on this device is pin RA1 (PGC2/DACOUT1/AD1AN7/...).
// Because DACOUT1 and AD1AN7 are the SAME pad, dacWrite(0,v) + analogRead(7)
// is an internal loopback with no external wiring (see AkDacAdcLoopback).
// Reference = AVDD: Vout ~= (value / 4096) * AVDD. 32-bit SFRs on AK.
#include <xc.h>

static bool s_dac_on = false;

void dacWrite(uint8_t channel, uint16_t value)
{
    (void)channel;                         // only DAC1 for now
    if (value > 4095) value = 4095;

    if (!s_dac_on) {
        _DACMD = 0;                        // PMD: enable DAC/comparator clock
        __asm__ volatile ("nop"); __asm__ volatile ("nop");

        // RA1 = DACOUT1: analog pad, digital output driver off (DAC drives it).
        ANSELAbits.ANSELA1 = 1;
        TRISAbits.TRISA1   = 1;

        DACCTRL1bits.ON      = 0;
        DACCTRL1bits.FCLKDIV = 0;          // DAC clock = FP (no divide)
        DACCTRL1bits.ON      = 1;          // enable the DAC/comparator module
        DAC1CONbits.DACOEN   = 1;          // route DAC1 to its output pin (RA1)
        DAC1CONbits.DACEN    = 1;          // enable DAC1
        s_dac_on = true;
    }
    DAC1DATbits.DACDAT = value;            // 12-bit output value (upper field)
}

#else  // ---- non-dsPIC33C stub ----
void dacWrite(uint8_t, uint16_t) {}
#endif
