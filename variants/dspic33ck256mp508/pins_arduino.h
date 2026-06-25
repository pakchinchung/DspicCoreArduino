#ifndef Pins_Arduino_h
#define Pins_Arduino_h
//
// Variant: dsPIC33CK256MP508 (GENERIC chip support).
//
// No fixed board pinout is assumed — you choose pins by their datasheet NAME
// (RA0..RE15, from cores/dspic/dspic_pins.h, already pulled in via Arduino.h):
//
//     #define LED_USER  RE6
//     pinMode(LED_USER, OUTPUT);
//     digitalWrite(LED_USER, HIGH);
//
// This device implements ports A..E (declared in variant.cpp's g_ports[]).

// CPU frequency = FOSC (FCY = F_CPU/2). The Tools > "Clock (CPU speed)" menu sets
// this via -DF_CPU={build.f_cpu}; the #ifndef lets that override and only falls
// back to 100 MHz (FOSC; FCY 50 MHz — the default/verified config) if unset.
#ifndef F_CPU
#define F_CPU            100000000UL
#endif

// LED_BUILTIN is required by stock sketches (e.g. the IDE's built-in Blink).
// There is no universal "user LED" on a bare chip, so this is only a sensible
// default — override it for your board:  #define LED_BUILTIN  <your pin>
#ifndef LED_BUILTIN
#define LED_BUILTIN      RA0
#endif

#endif /* Pins_Arduino_h */
