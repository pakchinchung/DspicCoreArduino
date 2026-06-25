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
// ---- dsPIC33A (AK) Timer1 1 ms tick ---------------------------------------
// AK register differences vs 33CK (verified from p33AK128MC106.h):
//   * enable bit is T1CONbits.ON (bit 15), NOT TON;
//   * T1 interrupt bits live in IFS1/IEC1/IPC6 (NOT IFS0/IEC0/IPC0);
//   * SFRs are 32-bit, so a 32-bit load of _millis_count is atomic (no DISI).
//   * vector name is still _T1Interrupt (DFP template confirms).
// AK uses a relocatable IVT (IVTBASE); whether _T1Interrupt vectors without extra
// setup is verified ON HARDWARE by the AkBlink test (delay() is millis-based, so
// if the ISR never fires the LEDs freeze instead of blink).
//
// CALIBRATION: PR1 assumes FCY = F_CPU/2. The AK clock/PLL isn't configured yet
// (DIM has an 8 MHz EC clock on CLKI/RC1), so the part runs on its power-on clock
// and the rate is approximate — but Timer1 still ticks, so timing progresses.
static volatile unsigned long _millis_count = 0;

extern "C" void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void)
{
    _millis_count++;
    IFS1bits.T1IF = 0;
}

void initTimer1(void)
{
    T1CON = 0;
    TMR1  = 0;
    T1CONbits.TCKPS = 1;                                     // 1:8 prescaler
    // AK Timer1 is clocked by the STANDARD-speed peripheral clock = FPB/2 = FCY/2
    // (= F_CPU/4), NOT FCY. (Verified by a millis-vs-host-timestamp self-test that
    // ran 2x slow with the FCY assumption.) So divide by an extra 2 vs the CK path.
    PR1 = (uint32_t)((F_CPU / 4UL / 8UL / 1000UL) - 1UL);    // ~1 ms @ Timer1=FCY/2

    IPC6bits.T1IP = 4;
    IFS1bits.T1IF = 0;
    IEC1bits.T1IE = 1;
    T1CONbits.ON  = 1;
}

unsigned long millis(void) { return _millis_count; }        // 32-bit load is atomic

unsigned long micros(void)
{
    unsigned long m   = _millis_count;
    unsigned long tmr = TMR1;
    // Timer1 @ FCY/2 with 1:8 prescaler ⇒ tick = 32e6/F_CPU µs; factor out 1e6 so
    // tmr*32 can't overflow 32-bit.
    return m * 1000UL + (tmr * 32UL) / (F_CPU / 1000000UL);
}

void delay(unsigned long ms)
{
    unsigned long start = millis();
    while ((millis() - start) < ms) {}
}

void delayMicroseconds(unsigned int us)
{
    while (us--) { __asm__ volatile ("nop"); }
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
    // Each Timer1 tick = 1/(FCY/8) s = 16000000/F_CPU us. Compute as 16/(F_CPU/1e6)
    // so the multiply can't overflow 32-bit (tmr*16000000 would, for tmr>~268).
    return m * 1000UL + ((unsigned long)tmr * 16UL / (F_CPU / 1000000UL));
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
