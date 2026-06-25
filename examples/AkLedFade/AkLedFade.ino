/*
 * AkLedFade - dspicArduino (dsPIC33AK128MC106 Curiosity GP DIM)
 *
 * Proof that AK PWM output is PPS-routable to ANY remappable pin: fades the RGB LED
 * AND all eight green user LEDs together with High-Speed PWM. There are only 4 PWM
 * generators, so each generator's H output is fanned out to ~3 pins (PPS lets many
 * pins select the same source); all four duties are driven together for a uniform
 * "breathing" fade. The RGB (RC2/RD0/RD2) keeps working alongside LED0..7
 * (RC3..RC10). Build with Tools > Clock = "100 MIPS".
 *   PG1H -> RC2(Red), RC3, RC4    PG2H -> RD0(Green), RC5, RC6
 *   PG3H -> RD2(Blue), RC7, RC8   PG4H -> RC9, RC10
 */
#include <xc.h>
#define PWM_PERIOD 0xF9F0UL

static void pwmInit(void)
{
    _PWMMD = 0;
    __asm__ volatile ("nop"); __asm__ volatile ("nop");

    // PWM master clock = Clock Gen 5 from PLL1 Out (PLL1 already up at 100 MIPS).
    CLK5CON = 0x00129500UL; CLK5DIV = 0;
    CLK5CONbits.OSWEN = 1;
    for (uint32_t i = 0; i < 300000UL && CLK5CONbits.OSWEN;   i++) { }
    for (uint32_t i = 0; i < 300000UL && !CLK5CONbits.CLKRDY; i++) { }

    // RC2..RC10 + RD0/RD2 = outputs; fan each generator H out to ~3 pins via PPS.
    TRISC &= ~0x07FCUL;                 // RC2..RC10 outputs
    TRISD &= ~0x0005UL;                 // RD0, RD2 outputs
    RPCONbits.IOLOCK = 0;
    _RP35R = 1; _RP36R = 1; _RP37R = 1; // PG1H (PWM1H) -> RC2(Red), RC3, RC4
    _RP49R = 3; _RP38R = 3; _RP39R = 3; // PG2H (PWM2H) -> RD0(Green), RC5, RC6
    _RP51R = 5; _RP40R = 5; _RP41R = 5; // PG3H (PWM3H) -> RD2(Blue), RC7, RC8
    _RP42R = 7; _RP43R = 7;             // PG4H (PWM4H) -> RC9, RC10
    RPCONbits.IOLOCK = 1;

    volatile uint32_t *con[4]  = { &PG1CON,  &PG2CON,  &PG3CON,  &PG4CON  };
    volatile uint32_t *ioc[4]  = { &PG1IOCON,&PG2IOCON,&PG3IOCON,&PG4IOCON};
    volatile uint32_t *evt[4]  = { &PG1EVT,  &PG2EVT,  &PG3EVT,  &PG4EVT  };
    volatile uint32_t *dc[4]   = { &PG1DC,   &PG2DC,   &PG3DC,   &PG4DC   };
    for (int i = 0; i < 4; i++) {
        *con[i] = 0x41000008UL;         // MPERSEL=1, master clock, ON later
        *ioc[i] = 0x02080200UL;         // PENH + PPSEN
        *evt[i] = 0x00000008UL;         // UPDTRG=1 (PGxDC write latches duty)
        *dc[i]  = 0;
    }
    MDC = 0; MPER = PWM_PERIOD;
    PCLKCON = 0x1UL;
    PG1CONbits.ON = 1; PG2CONbits.ON = 1; PG3CONbits.ON = 1; PG4CONbits.ON = 1;
}

static void setAll(uint32_t v)
{
    uint32_t d = v & 0x000FFFF0UL;
    PG1DC = d; PG2DC = d; PG3DC = d; PG4DC = d;
}

void setup(void) { pwmInit(); }

// Perceptual (gamma ~2) breathing: duty = level^2 scaled to the period, 256 steps
// -> smooth to the eye (linear duty steps look steppy at the low end).
static uint32_t gamma(uint32_t level) { return (level * level * PWM_PERIOD) / (255UL * 255UL); }

void loop(void)
{
    for (uint32_t l = 0;   l <= 255; l++) { setAll(gamma(l)); delay(6); }
    for (uint32_t l = 255; l > 0;    l--) { setAll(gamma(l)); delay(6); }
}
