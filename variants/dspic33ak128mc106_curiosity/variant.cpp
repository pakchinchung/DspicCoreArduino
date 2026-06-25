// dsPIC33AK128MC106 — board-level hardware init + port table.
// VARIANT: Curiosity Platform Development Board (EV74H48A) + dsPIC33AK128MC106
// General Purpose DIM (EV02G02A). Same chip g_ports[] as the generic variant;
// kept separate so the board's pins_arduino.h (LED0..7/SW1..3/POT) and any
// future board-specific clock init (8 MHz EC on CLKI/RC1) live here.
//
// dsPIC33A is a 32-bit-register architecture: GPIO SFRs are uint32_t, so the
// per-port table here uses 32-bit pointers (dspic_sfr_t resolves to uint32_t on
// __dsPIC33A__; see cores/dspic/dspic_pins.h). This device implements ports A..D.

#include "Arduino.h"
#include "wiring.h"          // initTimer1()
#include <xc.h>

// ---- Configuration bits (dsPIC33A setting names differ from 33CK) --
#pragma config WDTEN  = SW          // watchdog software-controlled (off at boot)
#pragma config JTAGEN = OFF
#pragma config ALTI2C2 = ON      // I2C2 -> alt pins ASDA2=RD8 / ASCL2=RD7 (Curiosity LCD)

// ---- Per-port register table (cores/dspic/dspic_pins.h) -----------
// 33AK128MC106 implements ports A..D. Index order matches PA..PG (A=0..D=3).
extern "C" const PortReg g_ports[] = {
    { &TRISA, &LATA, &PORTA, &ANSELA, &CNPUA },   // PA
    { &TRISB, &LATB, &PORTB, &ANSELB, &CNPUB },   // PB
    { &TRISC, &LATC, &PORTC, &ANSELC, &CNPUC },   // PC
    { &TRISD, &LATD, &PORTD, &ANSELD, &CNPUD },   // PD
};
extern "C" const uint8_t NUM_PORTS = (uint8_t)(sizeof(g_ports) / sizeof(g_ports[0]));

// ---- Clock: FRC + PLL1, speed chosen by Tools > "Clock (CPU speed)" ----------
// dsPIC33A: FCY = FPLLO = FOSC (1:1, NO /2 unlike CK). Our F_CPU convention is
// F_CPU = 2*FCY, so F_CPU=200 MHz => FCY 100 MHz (100 MIPS). Sequence per the AK
// datasheet (DS70005539 Example 12-4); waits are bounded so a non-lock can't hang.
static void initClock(void)
{
#if F_CPU == 200000000UL || F_CPU == 400000000UL
    // FRC(8 MHz)+PLL1, M=125, N1=1, N2=5 -> FVCO=1 GHz. N3 sets the speed:
    //   N3=2 -> FPLLO=FCY=100 MHz; the 200 MIPS option uses N3=1 -> 200 MHz.
    PLL1CONbits.ON       = 1;        // enable PLL generator
    PLL1CONbits.NOSC     = 1;        // PLL input = FRC
    PLL1DIVbits.PLLPRE   = 1;        // N1
    PLL1DIVbits.PLLFBDIV = 125;      // M  -> FVCO = 8*125 = 1000 MHz
    PLL1DIVbits.POSTDIV1 = 5;        // N2
#if F_CPU == 400000000UL
    PLL1DIVbits.POSTDIV2 = 1;        // N3 -> FPLLO = FCY = 200 MHz (200 MIPS)
#else
    PLL1DIVbits.POSTDIV2 = 2;        // N3 -> FPLLO = FCY = 100 MHz (100 MIPS)
#endif
    PLL1CONbits.PLLSWEN  = 1;        // apply input+feedback dividers
    for (uint32_t i = 0; i < 300000UL && PLL1CONbits.PLLSWEN;  i++) { }
    PLL1CONbits.FOUTSWEN = 1;        // apply output dividers
    for (uint32_t i = 0; i < 300000UL && PLL1CONbits.FOUTSWEN; i++) { }
    PLL1CONbits.OSWEN    = 1;        // start PLL
    for (uint32_t i = 0; i < 300000UL && PLL1CONbits.OSWEN;    i++) { }
    for (uint32_t i = 0; i < 300000UL && !OSCCTRLbits.PLL1RDY; i++) { }
    // Point the system clock generator (CLKGEN1, always ON) at PLL1 FOUT.
    CLK1CONbits.NOSC  = 5;           // PLL1 FOUT
    CLK1CONbits.OSWEN = 1;
    for (uint32_t i = 0; i < 300000UL && CLK1CONbits.OSWEN;    i++) { }
    for (uint32_t i = 0; i < 300000UL && !CLK1CONbits.CLKRDY;  i++) { }
#else
    // Power-on clock (FRC, ~5 MHz FCY). No PLL.
    (void)0;
#endif
}

// ---- initVariant — entry point called from main() ------------------
extern "C" void initVariant(void)
{
    initClock();    // PLL per the Clock menu (or power-on clock by default)
    initTimer1();   // 1 ms system tick
}
