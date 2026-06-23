#ifndef Arduino_h
#define Arduino_h

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>

#include "binary.h"
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

// ---- timing ----
unsigned long millis(void);
unsigned long micros(void);
void          delay(unsigned long ms);
void          delayMicroseconds(unsigned int us);

// ---- interrupts ----
void interrupts(void);
void noInterrupts(void);

// ---- exposed by user sketch (weak defaults in hooks.c) ----
void setup(void);
void loop(void);

#ifdef __cplusplus
} // extern "C"
#endif

// ---- C++-only includes ----
#ifdef __cplusplus
#include "WString.h"
#include "HardwareSerial.h"
#include "Stream.h"
#include "Print.h"
#endif

#endif /* Arduino_h */
