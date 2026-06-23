// dsPIC33AK128MC106 — board-level hardware init + port table.
// Called from main() before setup().
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

// ---- Per-port register table (cores/dspic/dspic_pins.h) -----------
// 33AK128MC106 implements ports A..D. Index order matches PA..PG (A=0..D=3).
extern "C" const PortReg g_ports[] = {
    { &TRISA, &LATA, &PORTA, &ANSELA, &CNPUA },   // PA
    { &TRISB, &LATB, &PORTB, &ANSELB, &CNPUB },   // PB
    { &TRISC, &LATC, &PORTC, &ANSELC, &CNPUC },   // PC
    { &TRISD, &LATD, &PORTD, &ANSELD, &CNPUD },   // PD
};
extern "C" const uint8_t NUM_PORTS = (uint8_t)(sizeof(g_ports) / sizeof(g_ports[0]));

// ---- initVariant — entry point called from main() ------------------
extern "C" void initVariant(void)
{
    // TODO: configure the dsPIC33A clock tree (OSCCTRL / PLLFBDIV / CLKDIV) to a
    // known FCY and update F_CPU in pins_arduino.h. Until then the device runs on
    // its default FRC and millis()/delay() timing is approximate.
    initTimer1();   // 1 ms system tick (best-effort until clock is set)
}
