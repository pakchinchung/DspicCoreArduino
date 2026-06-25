/*
 * tone() / noTone() - dspicArduino.
 *
 * Non-blocking square-wave generator built on an SCCP module (CCP1) used as a
 * 16-bit timer: its period-match interrupt (_CCT1Interrupt) toggles the output
 * pin, so the tone keeps playing in the background like the AVR core. CCP1 and
 * its CCT1 timer interrupt exist on both dsPIC33CK and dsPIC33AK with the same
 * register names, so one code path serves both. The timebase runs at FCY
 * (= F_CPU/2); a prescaler (1:1..1:64) keeps the period count in 16 bits down to
 * ~20 Hz. tone(pin,freq[,ms]); noTone(pin) stops it. Only one tone at a time.
 */
#include "Arduino.h"

#if defined(__dsPIC33C__) || defined(__dsPIC33A__)
#include <xc.h>

// SCCP1 register names differ: AK has 32-bit CCP1CON1/PR/TMR with an "ON" bit;
// CK splits them into 16-bit CCP1CON1L/H, CCP1PRL, CCP1TMRL with a "CCPON" bit.
#if defined(__dsPIC33A__)
  #define T_CON     CCP1CON1
  #define T_ON      CCP1CON1bits.ON
  #define T_TMRPS   CCP1CON1bits.TMRPS
  #define T_PR      CCP1PR
  #define T_TMR     CCP1TMR
#else  // __dsPIC33C__
  #define T_CON     CCP1CON1L
  #define T_ON      CCP1CON1Lbits.CCPON
  #define T_TMRPS   CCP1CON1Lbits.TMRPS
  #define T_PR      CCP1PRL
  #define T_TMR     CCP1TMRL
#endif

static volatile uint8_t  s_tonePin   = 255;
static volatile uint8_t  s_toneState = 0;
static volatile uint32_t s_toggles   = 0;     // remaining toggles, 0 = play forever

void noTone(uint8_t pin)
{
    (void)pin;
    T_ON = 0;
    _CCT1IE = 0;
    if (s_tonePin != 255) digitalWrite(s_tonePin, LOW);
    s_tonePin = 255;
}

void tone(uint8_t pin, unsigned int frequency, unsigned long duration)
{
    if (frequency == 0) { noTone(pin); return; }

    // SCCP timebase (CLKSEL=Tcy): measured FCY/2 on AK, FCY on CK (FCY = F_CPU/2).
#if defined(__dsPIC33A__)
    uint32_t clk = F_CPU / 4UL;
#else
    uint32_t clk = F_CPU / 2UL;
#endif
    uint32_t count = clk / (2UL * frequency);     // timer ticks per half-period
    uint8_t  ps    = 0;                           // prescale: 0=1:1,1=1:4,2=1:16,3=1:64
    while (count > 65535UL && ps < 3) { ps++; count >>= 2; }
    if (count < 1)      count = 1;
    if (count > 65535UL) count = 65535UL;

    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    s_tonePin   = pin;
    s_toneState = 0;
    // toggles needed for the requested duration (2 edges per cycle)
    s_toggles = duration ? ((uint32_t)frequency * 2UL * duration / 1000UL) : 0;

    _CCP1MD = 0;                                  // enable SCCP1 module clock (PMD)
    __asm__ volatile ("nop"); __asm__ volatile ("nop");
    T_CON   = 0;                                  // 16-bit timer, CLKSEL=Tcy, OFF
    T_TMRPS = ps;
    T_PR    = count - 1;                          // period -> CCT1 interrupt on match
    T_TMR   = 0;
    _CCT1IF = 0; _CCT1IP = 3; _CCT1IE = 1;
    T_ON    = 1;
}

extern "C" void __attribute__((interrupt, no_auto_psv)) _CCT1Interrupt(void)
{
    _CCT1IF = 0;
    s_toneState ^= 1;
    if (s_tonePin != 255) digitalWrite(s_tonePin, s_toneState);
    if (s_toggles && --s_toggles == 0) {          // duration elapsed -> stop
        T_ON = 0;
        _CCT1IE = 0;
        if (s_tonePin != 255) digitalWrite(s_tonePin, LOW);
        s_tonePin = 255;
    }
}

#else  // ---- other families: stub ----
void tone(uint8_t, unsigned int, unsigned long) {}
void noTone(uint8_t) {}
#endif
