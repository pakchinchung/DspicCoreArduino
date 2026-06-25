#ifndef Arduino_h
#define Arduino_h

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>

#include "binary.h"
#include "avr/pgmspace.h"   // PROGMEM / pgm_read_* compat for stock libraries
#include "dspic_pins.h"     // native-name pin model (RA0..RG15, PortReg, g_ports)
#include "pins_arduino.h"   // pulled from variant/ (board pin table + aliases)

#ifdef __cplusplus
extern "C" {
#endif

// ---- pin modes & levels ----
#define INPUT           0
#define OUTPUT          1
#define INPUT_PULLUP    2

#define LOW             0
#define HIGH            1

// ---- bit order (SPI, shiftIn/Out) ----
#define LSBFIRST        0
#define MSBFIRST        1

// ---- print number bases ----
#define DEC             10
#define HEX             16
#define OCT             8
#define BIN             2

// ---- math / bits ----
#define PI              3.1415926535897932384626433832795
#define HALF_PI         1.5707963267948966192313216916398
#define TWO_PI          6.283185307179586476925286766559
#define DEG_TO_RAD      0.017453292519943295769236907684886
#define RAD_TO_DEG      57.295779513082320876798154814105

// These collide with <math.h>/<stdlib.h> function-like macros; the Arduino
// API intentionally provides its own, so undefine first (mirrors AVR core).
#undef min
#undef max
#undef abs
#undef round
#define min(a,b)        ((a)<(b)?(a):(b))
#define max(a,b)        ((a)>(b)?(a):(b))
#define abs(x)          ((x)>0?(x):-(x))
#define constrain(amt,lo,hi) ((amt)<(lo)?(lo):((amt)>(hi)?(hi):(amt)))
#define round(x)        ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#define radians(deg)    ((deg)*DEG_TO_RAD)
#define degrees(rad)    ((rad)*RAD_TO_DEG)
#define sq(x)           ((x)*(x))

#define bit(b)          (1UL << (b))
#define bitRead(v,b)    (((v) >> (b)) & 1)
#define bitSet(v,b)     ((v) |=  (1UL << (b)))
#define bitClear(v,b)   ((v) &= ~(1UL << (b)))
#define bitWrite(v,b,x) ((x) ? bitSet(v,b) : bitClear(v,b))
#define lowByte(w)      ((uint8_t) ((w) & 0xff))
#define highByte(w)     ((uint8_t) ((w) >> 8))

// ---- type aliases matching Arduino conventions ----
typedef unsigned int  word;
typedef bool          boolean;
typedef uint8_t       byte;

// ---- digital I/O ----
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int  digitalRead(uint8_t pin);

// ---- analog I/O ----
int  analogRead(uint8_t pin);
void analogWrite(uint8_t pin, int val);
void analogReadResolution(uint8_t bits);
void analogWriteResolution(uint8_t bits);
void analogReference(uint8_t mode);
void dacWrite(uint8_t channel, uint16_t value);   // dsPIC DAC (0..4095)

// ---- timing ----
unsigned long millis(void);
unsigned long micros(void);
void          delay(unsigned long ms);
void          delayMicroseconds(unsigned int us);

// ---- interrupts ----
void interrupts(void);
void noInterrupts(void);

// ---- pin-change interrupts (attachInterrupt) ----
// Edge modes (values follow the AVR convention; libraries use the names).
// LOW/HIGH level-trigger is NOT supported (the CN hardware is edge-only).
#define CHANGE   1
#define FALLING  2
#define RISING   3

typedef void (*voidFuncPtr)(void);

// Our pin model already uses native pin ids, so a "pin" IS the interrupt id —
// digitalPinToInterrupt() is the identity (provided for sketch portability).
#define digitalPinToInterrupt(p)  (p)

void attachInterrupt(uint8_t pin, voidFuncPtr callback, int mode);
void detachInterrupt(uint8_t pin);

// ---- advanced I/O ----
void          shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val);
uint8_t       shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder);
unsigned long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout);
unsigned long pulseInLong(uint8_t pin, uint8_t state, unsigned long timeout);
void          tone(uint8_t pin, unsigned int frequency, unsigned long duration);
void          noTone(uint8_t pin);

// ---- misc ----
long map(long x, long in_min, long in_max, long out_min, long out_max);
void yield(void);   // weak no-op (single-threaded); libraries may override

// ---- exposed by user sketch (weak defaults in hooks.c) ----
void setup(void);
void loop(void);

#ifdef __cplusplus
} // extern "C"
#endif

// ---- C++-only declarations ----
// random() is overloaded, which is illegal under C linkage, so it lives outside
// the extern "C" block (matches the AVR core). Sketches/libraries are C++.
#ifdef __cplusplus
long random(long howbig);
long random(long howsmall, long howbig);
void randomSeed(unsigned long seed);

// Default-argument conveniences (sketches are C++): pulseIn(pin,state) uses a 1 s
// timeout; tone(pin,freq) plays until noTone().
inline unsigned long pulseIn(uint8_t pin, uint8_t state) { return pulseIn(pin, state, 1000000UL); }
inline unsigned long pulseInLong(uint8_t pin, uint8_t state) { return pulseInLong(pin, state, 1000000UL); }
inline void tone(uint8_t pin, unsigned int frequency) { tone(pin, frequency, 0UL); }
#endif

// ---- C++-only includes ----
#ifdef __cplusplus
#include "WString.h"
#include "HardwareSerial.h"
#include "Stream.h"
#include "Print.h"
#endif

#endif /* Arduino_h */
