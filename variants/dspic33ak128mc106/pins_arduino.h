#ifndef Pins_Arduino_h
#define Pins_Arduino_h
//
// Variant: dsPIC33AK128MC106 — GENERIC chip support (no board assumptions).
//
// Pins are referenced by their datasheet NAME (RA0..RD15, from
// cores/dspic/dspic_pins.h via Arduino.h). This device implements ports A..D.
//   #define LED  RC3
//   pinMode(LED, OUTPUT);
//
// For the Curiosity Platform board + dsPIC33AK128MC106 GP DIM, which has named
// LEDs/buttons/potentiometer, select  Tools > Pin mapping > "Curiosity GP DIM"
// (that's the dspic33ak128mc106_curiosity variant) instead.
//
// NOTE: dsPIC33A is experimental — digital I/O + delay work on real silicon;
// the HAL drivers (Serial/SPI/Wire/ADC/PWM) are not yet ported to AK. The AK
// clock/PLL is not configured (runs on the power-on clock), so millis()/delay()
// timing is approximate. See docs/PROJECT_STATUS.md.

// #ifndef so a future Tools "Clock" menu can override via -DF_CPU={build.f_cpu}.
#ifndef F_CPU
#define F_CPU            20000000UL    // ~AK power-on clock (FOSC); see boards.txt
#endif

// Bare chip has no designated LED; this is only a sensible default so stock
// sketches that reference LED_BUILTIN still compile. Override for your board.
#ifndef LED_BUILTIN
#define LED_BUILTIN      RA0
#endif

#endif /* Pins_Arduino_h */
