// dsPIC33CK256MP508 (generic) — board-level hardware init + port table.
// Called from main() before setup(). Pins are referenced by native name
// (RA0..RE15); this file just publishes which ports the chip implements.

#include "Arduino.h"
#include "wiring.h"          // initTimer1()
#include <xc.h>

// ---- Configuration bits -------------------------------------------
// Safe defaults: FRC + PLL, watchdog off, JTAG off. Users may override with
// their own #pragma config in the sketch.
#pragma config FNOSC    = FRC         // start on FRC; firmware switches to PLL
#pragma config IESO     = OFF
#pragma config FCKSM    = CSECMD      // clock switching enabled, monitor off
#pragma config OSCIOFNC = ON          // OSC2 pin is GPIO
#pragma config FWDTEN   = ON_SW       // WDT software-controlled (off at boot)
#pragma config JTAGEN   = OFF
#pragma config ICS      = PGD2        // ICSP on PGD2/PGC2 (DM330030 debugger)

// ---- Per-port register table (cores/dspic/dspic_pins.h) -----------
// dsPIC33CK256MP508 implements ports A..E (no F/G). Index order MUST match
// the PA..PG enum (A=0..E=4); absent ports get NULL entries.
extern "C" const PortReg g_ports[] = {
    { &TRISA, &LATA, &PORTA, &ANSELA, &CNPUA },   // PA
    { &TRISB, &LATB, &PORTB, &ANSELB, &CNPUB },   // PB
    { &TRISC, &LATC, &PORTC, &ANSELC, &CNPUC },   // PC
    { &TRISD, &LATD, &PORTD, &ANSELD, &CNPUD },   // PD
    { &TRISE, &LATE, &PORTE, &ANSELE, &CNPUE },   // PE
};
extern "C" const uint8_t NUM_PORTS = (uint8_t)(sizeof(g_ports) / sizeof(g_ports[0]));

// ---- Clock: FRC(8MHz) + PLL -> FOSC 200 MHz, FCY 100 MHz ----------
static void initClock(void)
{
    // PLL: FPLLIN = 8 MHz (N1=1), FVCO = 800 MHz (M=100),
    //      FPLLO  = 200 MHz (POST1=4, POST2=1) -> FOSC 200 MHz, FCY 100 MHz.
    CLKDIVbits.PLLPRE   = 1;     // N1
    PLLFBDbits.PLLFBDIV = 100;   // M
    PLLDIVbits.POST1DIV = 4;     // N2
    PLLDIVbits.POST2DIV = 1;     // N3

    __builtin_write_OSCCONH(0x01);   // NOSC = FRCPLL
    __builtin_write_OSCCONL(OSCCONL | 0x01);   // request switch

    // Wait for the switch + PLL lock, but BOUNDED so a simulator (where LOCK may
    // never assert) cannot hang forever; on real silicon lock is reached early.
    for (uint16_t i = 0; i < 20000; i++) {
        if (OSCCONbits.OSWEN == 0 && OSCCONbits.LOCK) break;
    }
}

// ---- initVariant — entry point called from main() ------------------
extern "C" void initVariant(void)
{
    initClock();
    initTimer1();   // 1 ms system tick for millis()/delay()
}
