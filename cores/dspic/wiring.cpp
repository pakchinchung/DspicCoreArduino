// millis(), micros(), delay(), delayMicroseconds()
//
// dsPIC33C (CK) uses a Timer1 1 ms ISR (below). dsPIC33A (AK) has a different
// interrupt-vector / timer SFR layout and an as-yet-unverified C++ runtime, so
// it gets a compile-safe, approximate fallback (no ISR) until the AK clock +
// timer are properly brought up — see docs/DSPIC33A_STATUS.md.

#include "Arduino.h"
#include "wiring.h"     // extern "C" initTimer1() prototype (linkage must match)
#include <xc.h>

#if defined(__dsPIC33A__)
// ---- dsPIC33A (AK) approximate timing — TODO: real Timer1 + clock ---------
static volatile unsigned long _millis_count = 0;

void initTimer1(void) { /* TODO(AK): IFS1/IEC1-based Timer1 ISR + clock tree */ }

unsigned long millis(void) { return _millis_count; }
unsigned long micros(void) { return _millis_count * 1000UL; }

void delayMicroseconds(unsigned int us)
{
    // Coarse busy-wait; precision pending the AK clock setup.
    while (us--) { __asm__ volatile ("nop"); }
}

void delay(unsigned long ms)
{
    while (ms--) { delayMicroseconds(1000); _millis_count++; }
}

void interrupts(void)   { __builtin_enable_interrupts();  }
void noInterrupts(void) { __builtin_disable_interrupts(); }

#else
// ---- dsPIC33C / classic 16-bit parts: Timer1 1 ms ISR ---------------------

// Ticked by the Timer1 ISR every 1 ms.
// volatile so the compiler never optimises reads away.
static volatile unsigned long _millis_count = 0;

// ---- Timer1 ISR --------------------------------------------------------
// Vector name matches XC-DSC/pic30-gcc convention for dsPIC33CK.
extern "C" void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void)
{
    _millis_count++;
    IFS0bits.T1IF = 0;   // clear interrupt flag
}

// ---- initTimer1 -------------------------------------------------------
// Called from initVariant().
// Configures Timer1 to fire every 1 ms.
// FCY = F_CPU / 2.  With prescaler 1:8 and F_CPU = 100 MHz:
//   FCY = 50 MHz,  tick = 1/(50MHz/8) = 160 ns
//   PR1 = 1ms / 160ns - 1 = 6249
void initTimer1(void)
{
    T1CON    = 0;
    TMR1     = 0;
    T1CONbits.TCKPS = 1;    // 1:8 prescaler

#if F_CPU == 100000000UL
    PR1 = 6249;
#elif F_CPU == 60000000UL
    PR1 = 3749;
#elif F_CPU == 40000000UL
    PR1 = 2499;
#else
    // Generic: FCY = F_CPU/2, 1:8 prescaler, 1ms period
    PR1 = (unsigned int)((F_CPU / 2UL / 8UL / 1000UL) - 1UL);
#endif

    IPC0bits.T1IP  = 4;     // priority 4 (mid-range)
    IFS0bits.T1IF  = 0;
    IEC0bits.T1IE  = 1;

    T1CONbits.TON  = 1;
}

// ---- Public API -------------------------------------------------------

unsigned long millis(void)
{
    unsigned long m;
    // Reading a 32-bit variable on a 16-bit CPU needs atomic protection.
    __builtin_disi(0x3FFF);     // disable interrupts for up to 16383 cycles
    m = _millis_count;
    __builtin_disi(0x0000);
    return m;
}

unsigned long micros(void)
{
    unsigned long m;
    unsigned int  tmr;
    __builtin_disi(0x3FFF);
    m   = _millis_count;
    tmr = TMR1;
    __builtin_disi(0x0000);
    // Each Timer1 tick = 1/(FCY/8) seconds.  Convert to microseconds.
    // tick_us = 8 * 1000000 / (F_CPU/2) = 16000000 / F_CPU
    return m * 1000UL + ((unsigned long)tmr * 16000000UL / F_CPU);
}

void delay(unsigned long ms)
{
    unsigned long start = millis();
    while ((millis() - start) < ms) {}
}

void delayMicroseconds(unsigned int us)
{
    // For short delays use the built-in cycle counter.
    // __delay_us() is provided by <libpic30.h> and compiled inline.
    // FCY must be defined — we define it in pins_arduino.h as F_CPU/2.
    while (us--) {
        // 1 µs at 100 MHz = 50 instruction cycles (FCY=50MHz).
        __asm__ volatile("repeat #49\n\tnop");
    }
}

void interrupts(void)   { __builtin_enable_interrupts();  }
void noInterrupts(void) { __builtin_disable_interrupts(); }

#endif /* __dsPIC33A__ */
