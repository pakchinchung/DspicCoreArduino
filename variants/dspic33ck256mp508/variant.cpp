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

// I2C1 pin routing (FDEVOPT.ALTI2C1) — compile-time, not runtime: I2C SCL/SDA
// are not PPS pins. Default = ALTERNATE pins RC8 (SDA) / RC9 (SCL). Build with
// -DDSPIC_I2C1_PRIMARY (Tools > "I2C1 pins" menu) to use primary RB8/RB9.
#if defined(DSPIC_I2C1_PRIMARY)
#pragma config ALTI2C1 = OFF          // I2C1 -> SDA1/SCL1 (RB9/RB8)
#else
#pragma config ALTI2C1 = ON           // I2C1 -> ASDA1/ASCL1 (RC8/RC9)
#endif

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

// ---- Clock: FRC(8 MHz) + PLL, speed chosen by Tools > "Clock (CPU speed)" ----
// The menu sets F_CPU (= FOSC); this picks the matching PLL dividers. For FRC-PLL
// modes the silicon halves the PLL output, so FOSC = FPLLO/2 and FCY = FOSC/2:
//   FCY = FPLLI*M/(N1*N2*N3)/4,  FPLLI = 8 MHz (FRC).
//   VCO = FPLLI*M/N1 must be 400-1600 MHz; FPLLI 8-64 MHz.
// Verified default: F_CPU=100 MHz -> FOSC 100 MHz, FCY 50 MHz.
static void initClock(void)
{
#if F_CPU == 8000000UL
    // 8 MHz FRC, no PLL. FNOSC=FRC is the power-on default, so nothing to switch.
    (void)0;
#else
  #if F_CPU == 200000000UL
    // FOSC 200 MHz / FCY 100 MHz (max): FVCO=800, FPLLO=400, /2 -> FOSC 200.
    CLKDIVbits.PLLPRE   = 1;     // N1
    PLLFBDbits.PLLFBDIV = 100;   // M  -> FVCO = 8*100 = 800 MHz
    PLLDIVbits.POST1DIV = 2;     // N2 -> FPLLO = 800/2 = 400 MHz -> FOSC 200
    PLLDIVbits.POST2DIV = 1;     // N3
  #else  // F_CPU == 100000000UL (default): FOSC 100 MHz / FCY 50 MHz
    CLKDIVbits.PLLPRE   = 1;     // N1
    PLLFBDbits.PLLFBDIV = 100;   // M  -> FVCO = 800 MHz
    PLLDIVbits.POST1DIV = 4;     // N2 -> FPLLO = 200 MHz -> FOSC 100
    PLLDIVbits.POST2DIV = 1;     // N3
  #endif

    __builtin_write_OSCCONH(0x01);             // NOSC = FRCPLL
    __builtin_write_OSCCONL(OSCCONL | 0x01);   // request switch

    // Bounded wait for switch + PLL lock (a sim may never assert LOCK).
    for (uint16_t i = 0; i < 20000; i++) {
        if (OSCCONbits.OSWEN == 0 && OSCCONbits.LOCK) break;
    }
#endif
}

// ---- initVariant — entry point called from main() ------------------
extern "C" void initVariant(void)
{
    initClock();
    initTimer1();   // 1 ms system tick for millis()/delay()
}
