#ifndef Pins_Arduino_h
#define Pins_Arduino_h
//
// Variant: dsPIC33AK128MC106 (dsPIC33A family, 32-bit GPIO SFRs).
//
// Pin model is by NATIVE NAME (RA0..RD15, from cores/dspic/dspic_pins.h via
// Arduino.h). This device has no on-board Arduino-standard header, so the
// integer aliases below are example mappings — adjust for your board.
//
// NOTE: dsPIC33A is a newer architecture. C++ compiles and links here, but the
// C++ runtime on the MPLAB X simulator is still under investigation
// (see docs/DSPIC33A_STATUS.md). Treat this variant as functional-but-unverified
// on real silicon until clock + runtime are validated.

// FRC default; PLL not yet configured in variant.cpp (see TODO there). This
// value only feeds the millis()/delay() timer math and must be corrected once
// the AK clock tree is set up.
#define F_CPU            100000000UL

// Example I/O aliases (no fixed board) — change to match your hardware.
#define LED_BUILTIN      RA0
#define D0               RA0
#define D1               RA1
#define D2               RB0
#define D3               RB1
#define D4               RC0
#define D5               RD0

#define NUM_DIGITAL_PINS   6
#define NUM_ANALOG_INPUTS  0

#endif /* Pins_Arduino_h */
