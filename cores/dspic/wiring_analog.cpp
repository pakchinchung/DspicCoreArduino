// analogRead() / analogReference() / analogReadResolution()  — dsPIC33CK ADC
// (12-bit SAR, shared core, software common trigger).
//
// analogRead(ch) takes an ADC channel number (AN0, AN1, ...). A variant may
// #define A0..An to the channels its header pins expose.
//
// dacWrite() / analogWrite() live in wiring_dac.cpp / wiring_pwm.cpp.

#include "Arduino.h"

#if defined(__dsPIC33C__)
#include "xc_compat.h"

static bool     s_adc_ready = false;
static uint8_t  s_adc_bits  = 10;          // Arduino default resolution

void analogReadResolution(uint8_t bits) { s_adc_bits = bits; }

static void adcInit()
{
    ADCON1Lbits.ADON   = 0;                // configure with module off
    ADCON1Hbits.FORM   = 0;                // unsigned integer result
    ADCON1Hbits.SHRRES = 3;                // shared core = 12-bit

    ADCON3Hbits.CLKSEL = 0;                // ADC clock = peripheral clock (FP)
    ADCON3Hbits.CLKDIV = 0;                // /1
    ADCON2Lbits.SHRADCS = 4;               // shared-core TAD divider
    // Shared-core sample (acquisition) time. ALL channels AN2.. (e.g. AN23 = the
    // pot) go through the shared core, so its sample-and-hold needs enough time
    // to charge from a high-impedance source. Default is ~2 TAD — far too short
    // for a pot, which makes the reading slew slowly toward the true value
    // (looks like a "laggy" pot). 31 TAD gives ample acquisition.
    ADCON2Hbits.SHRSAMC = 31;
    ADCON3Lbits.REFSEL = 0;                // Vref = AVDD / AVSS

    // Dedicated cores 0 & 1 (handle AN0 / AN1): 12-bit, clock + sample time.
    ADCORE0Hbits.RES = 3; ADCORE0Hbits.ADCS = 4; ADCORE0Lbits.SAMC = 10;
    ADCORE1Hbits.RES = 3; ADCORE1Hbits.ADCS = 4; ADCORE1Lbits.SAMC = 10;

    ADCON1Lbits.ADON = 1;                  // enable module FIRST, then power cores

    ADCON5Lbits.C0PWR = 1;                 // power dedicated + shared cores
    ADCON5Lbits.C1PWR = 1;
    ADCON5Lbits.SHRPWR = 1;
    for (uint16_t t = 0; t < 60000; t++) {
        if (ADCON5Lbits.C0RDY && ADCON5Lbits.C1RDY && ADCON5Lbits.SHRRDY) break;
    }
    ADCON3Hbits.C0EN  = 1;                 // analog-enable all cores
    ADCON3Hbits.C1EN  = 1;
    ADCON3Hbits.SHREN = 1;
    s_adc_ready = true;
}

int analogRead(uint8_t channel)
{
    if (!s_adc_ready) adcInit();

    ADCON3Lbits.CNVCHSEL = channel;        // pick the channel
    ADCON3Lbits.CNVRTCH  = 1;              // software common-conversion trigger

    // Wait (bounded) for this channel's result-ready flag.
    uint16_t guard = 0;
    if (channel < 16) { while (!(ADSTATL & (1u << channel))        && ++guard) { } }
    else              { while (!(ADSTATH & (1u << (channel - 16))) && ++guard) { } }

    uint16_t raw = (&ADCBUF0)[channel];    // 12-bit result (0..4095)

    if (s_adc_bits >= 12) return raw;
    return raw >> (12 - s_adc_bits);       // scale to requested resolution (def 10-bit)
}

void analogReference(uint8_t) { /* only AVDD supported for now */ }

#elif defined(__dsPIC33A__)
// ---- dsPIC33AK ADC (ADC1, per-channel; software-triggered single-shot) -------
// analogRead(ch) takes an analog input number ANch (e.g. the Curiosity pot is on
// AD1AN6 -> analogRead(6)). The AK ADC needs its own clock generator (CLKGEN6);
// we source it from the 8 MHz FRC so it works on any system-clock setting (slow
// but fine for analog reads). Sequence per datasheet DS70005539 Example 15-1.
#include <xc.h>

static bool    s_adc_ready = false;
static uint8_t s_adc_bits  = 10;          // Arduino default resolution

void analogReadResolution(uint8_t bits) { s_adc_bits = bits; }

static void adcInit()
{
    _ADC1MD = 0;                          // enable ADC1 module clock (PMD)
    __asm__ volatile ("nop"); __asm__ volatile ("nop");

    // ADC high-speed clock = CLKGEN6, sourced from FRC (8 MHz).
    CLK6CONbits.ON   = 1;
    CLK6CONbits.NOSC = 1;                 // FRC
    CLK6CONbits.OSWEN = 1;
    for (uint32_t i = 0; i < 300000UL && CLK6CONbits.OSWEN;  i++) { }
    for (uint32_t i = 0; i < 300000UL && !CLK6CONbits.CLKRDY; i++) { }

    AD1CONbits.ON = 1;                    // enable ADC; it auto-calibrates
    for (uint32_t i = 0; i < 300000UL && !AD1CONbits.ADRDY; i++) { }
    s_adc_ready = true;
}

int analogRead(uint8_t channel)
{
    if (!s_adc_ready) adcInit();

    AD1CH0CONbits.MODE    = 0;            // single-shot
    AD1CH0CONbits.TRG1SRC = 1;            // software trigger (AD1SWTRG)
    AD1CH0CONbits.PINSEL  = channel & 0x0F;   // analog input ANx (single-ended)
    AD1CH0CONbits.SAMC    = 7;            // long sample time (pot is high-impedance)

    AD1SWTRGbits.CH0TRG = 1;              // start the conversion
    uint32_t guard = 0;
    while (!AD1STATbits.CH0RDY && ++guard < 300000UL) { }

    uint16_t raw = (uint16_t)(AD1CH0DATA & 0x0FFF);   // 12-bit, right-aligned

    if (s_adc_bits >= 12) return raw;
    return raw >> (12 - s_adc_bits);      // scale to requested resolution (def 10-bit)
}

void analogReference(uint8_t) { /* only AVDD supported for now */ }

#else  // ---- other families stub ----
void analogReadResolution(uint8_t) {}
int  analogRead(uint8_t) { return 0; }
void analogReference(uint8_t) {}
#endif
