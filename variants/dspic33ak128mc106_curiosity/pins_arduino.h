#ifndef Pins_Arduino_h
#define Pins_Arduino_h
//
// Variant: dsPIC33AK128MC106 on the Curiosity Platform Development Board
// (EV74H48A) + dsPIC33AK128MC106 General Purpose DIM (EV02G02A).
// Pin model is by NATIVE NAME (RA0..RD15, from cores/dspic/dspic_pins.h via
// Arduino.h). Board pin mapping below is from the DIM info sheet (DS70005556)
// cross-referenced with the demo board net names — see docs/PROJECT_STATUS.md §4.
//
// NOTE: dsPIC33A C++ compiles/links and digital I/O + Timer1 work; the HAL
// drivers (Serial/SPI/Wire/ADC/PWM) are not yet ported to AK (still 33C-only).

// F_CPU = FOSC, empirically calibrated to the AK power-on clock (~10 MHz). The
// DIM has an 8.000 MHz external clock oscillator (EC) on CLKI/RC1; once
// variant.cpp initClock() PLLs it to a known FOSC, set this to that FOSC.
// #ifndef so a future Tools "Clock" menu can override via -DF_CPU={build.f_cpu}.
#ifndef F_CPU
#define F_CPU            20000000UL
#endif

// ---- Board peripherals (DIM info sheet DS70005556) ------------------------
// 8 general-purpose green LEDs (LED0 = rightmost / LSB).
#define LED0   RC3
#define LED1   RC4
#define LED2   RC5
#define LED3   RC6
#define LED4   RC7
#define LED5   RC8
#define LED6   RC9
#define LED7   RC10

// RGB LED (each driveable by PWM).
#define LED_RED    RC2
#define LED_GREEN  RD0
#define LED_BLUE   RD2

// Push buttons S1..S3: active-LOW, external pull-up to Board_VDD (press = 0).
#define SW1    RB5
#define SW2    RB4
#define SW3    RA6

// 10k potentiometer wiper (analog AD1AN6).
#define POT    RA7

// LED_BUILTIN (stock Blink): use green LED0.
#ifndef LED_BUILTIN
#define LED_BUILTIN  LED0
#endif

// Numeric aliases D0..D7 map to the 8 green LEDs for stock-sketch portability.
#define D0  LED0
#define D1  LED1
#define D2  LED2
#define D3  LED3
#define D4  LED4
#define D5  LED5
#define D6  LED6
#define D7  LED7

#define NUM_DIGITAL_PINS   8
#define NUM_ANALOG_INPUTS  1

// ---- UART routing for the two USB-serial back-channels (when Serial is ported
// to AK). PPS input/output use RPn numbers; AK RPn map differs from CK.
//   COM (MCP2221A "USB-UART"):  MCU TX = RD3 (RP52),  MCU RX = RD1 (RP50)
//   COM (PKOB4 back-channel):   MCU TX = RD10(RP59),  MCU RX = RD9 (RP58)

#endif /* Pins_Arduino_h */
