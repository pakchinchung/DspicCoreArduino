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

#elif defined(__dsPIC33A__)
// ---- dsPIC33AK High-Speed PWM (PWM generators PGx) ---------------------------
// Drives the Curiosity RGB LED, whose colors are on dedicated PWMxH pins:
//   Blue  = RD2 / PWM1H (PG1)   Green = RD0 / PWM2H (PG2)   Red = RC2 / PWM4H (PG4)
// PWM master clock = Standard-Speed Peripheral Clock (MCLKSEL=0) — always
// available regardless of the system-clock menu. The duty PGxDC/PGxPER ratio sets
// brightness; absolute frequency isn't critical for an LED. (analogWrite for other
// pins is a no-op until more PWM pins are mapped.)
#include <xc.h>

static bool    s_pwm_on   = false;
static uint8_t s_pwm_bits = 8;
#define PWM_PERIOD 0xF9F0u                 // 63984 (1/16-clock units), as MCC uses

void analogWriteResolution(uint8_t bits) { s_pwm_bits = bits; }

// Configure one generator (ON stays 0 — enabled for all at the very end).
// Values verbatim from the HW-VERIFIED MCC project (Training.X):
//   PGxCON   = 0x41000008 — independent-edge, master clock (CLKSEL=1), self-
//              trigger, UPDMOD immediate, **MPERSEL=1** (period from MASTER MPER).
//   PGxIOCON = 0x02080200 — **PENH=1, PPSEN=1** (output routed via PPS), PENL=0.
// The period comes from MPER, so PGxPER is left 0; brightness = PGxDC / MPER.
static void pgConfig(volatile uint32_t *con, volatile uint32_t *iocon,
                     volatile uint32_t *evt, volatile uint32_t *per,
                     volatile uint32_t *dc,  volatile uint32_t *stat)
{
    *con   = 0x41000008UL;                 // MPERSEL=1; ON (bit15) stays 0 here
    *iocon = 0x02080200UL;                 // PENH + PPSEN (PWM out via PPS)
    *evt   = 0x00000008UL;                 // UPDTRG=1: writing PGxDC latches new duty
    *per   = 0;                            // unused — period is the master MPER
    *dc    = 0;
    *stat  = 0;
}

static void pwmInit()
{
    // ---- (1) CLOCK: PWM master clock = Clock Generator 5. CLK5CON=0x129500 selects
    // NOSC=2 which, for THIS generator, is "PLL1 Out output" (PLL1 FOUT) — the NOSC
    // encoding is per-generator. Values verbatim from the HW-verified Training.X
    // CLOCK_Initialize. Needs the PLL running (Tools > Clock = "100 MIPS"); PWM freq
    // = FOUT / MPER. (Our FOUT=100 MHz so ~1.56 kHz; Training.X's FOUT=400 MHz.)
    _PWMMD = 0;                            // enable the HS PWM module clock (PMD)
    __asm__ volatile ("nop"); __asm__ volatile ("nop");
    CLK5CON = 0x00129500UL;                // NOSC=2 = PLL1 Out, ON
    CLK5DIV = 0x00000000UL;
    CLK5CONbits.OSWEN = 1;
    for (uint32_t i = 0; i < 300000UL && CLK5CONbits.OSWEN;   i++) { }
    for (uint32_t i = 0; i < 300000UL && !CLK5CONbits.CLKRDY; i++) { }

    // ---- (2) PINS: AK PWM output IS PPS-routed (PPSEN=1). Make each RGB pad a
    // digital output and tie its RPn to a PWM-generator H output (codes from the
    // RPnR table: PWM1H=1, PWM2H=3, PWM3H=5). Mapping per Training.X:
    //   RC2 (RP35) <- PWM1H (PG1) = Red,  RD0 (RP49) <- PWM2H (PG2) = Green,
    //   RD2 (RP51) <- PWM3H (PG3) = Blue.  (PPS is unlocked out of reset on AK.)
    TRISCbits.TRISC2 = 0;
    TRISDbits.TRISD0 = 0;
    TRISDbits.TRISD2 = 0;
    RPCONbits.IOLOCK = 0;                  // unlock PPS
    _RP35R = 1;                            // RC2 <- PWM1H (Red)
    _RP49R = 3;                            // RD0 <- PWM2H (Green)
    _RP51R = 5;                            // RD2 <- PWM3H (Blue)
    RPCONbits.IOLOCK = 1;                  // lock PPS

    // ---- (3) PWM: configure generators (not enabled) ...
    pgConfig(&PG1CON, &PG1IOCON, &PG1EVT, &PG1PER, &PG1DC, &PG1STAT);   // PG1 -> RC2 (Red)
    pgConfig(&PG2CON, &PG2IOCON, &PG2EVT, &PG2PER, &PG2DC, &PG2STAT);   // PG2 -> RD0 (Green)
    pgConfig(&PG3CON, &PG3IOCON, &PG3EVT, &PG3PER, &PG3DC, &PG3STAT);   // PG3 -> RD2 (Blue)

    // ... master period (generators use it via MPERSEL=1) ...
    MDC  = 0;
    MPER = PWM_PERIOD;

    // ... select the PWM master clock (PCLKCON=0x1), THEN enable all gens LAST.
    PCLKCONbits.MCLKSEL = 1;               // PWM master clock = Clock Generator 5
    PCLKCONbits.DIVSEL  = 0;
    PG1CONbits.ON = 1;
    PG2CONbits.ON = 1;
    PG3CONbits.ON = 1;
    s_pwm_on = true;
}

void analogWrite(uint8_t pin, int value)
{
    if (!s_pwm_on) pwmInit();

    uint16_t maxv = (uint16_t)((1u << s_pwm_bits) - 1);
    if (value < 0) value = 0;
    if (value > (int)maxv) value = maxv;
    // duty = value/maxv * MPER, masked to a multiple of 16 (as PWM_DutyCycleSet does)
    uint32_t dc = (((uint32_t)value * PWM_PERIOD) / maxv) & 0x000FFFF0UL;

    switch (pin) {
    case RC2: PG1DC = dc; break;           // PWM1H (Red)
    case RD0: PG2DC = dc; break;           // PWM2H (Green)
    case RD2: PG3DC = dc; break;           // PWM3H (Blue)
    default: break;                        // pin not PWM-mapped on AK yet
    }
}

#else  // ---- other families stub ----
void analogWriteResolution(uint8_t) {}
void analogWrite(uint8_t, int) {}
#endif
