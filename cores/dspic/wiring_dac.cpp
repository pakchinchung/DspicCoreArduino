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

#else  // ---- non-dsPIC33C stub ----
void dacWrite(uint8_t, uint16_t) {}
#endif
