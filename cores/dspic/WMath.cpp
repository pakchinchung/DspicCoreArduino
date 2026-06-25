// WMath.cpp — Arduino math/random helpers (map, random, randomSeed).
//
// map() is the integer remap countless sketches/libraries call. random()/
// randomSeed() wrap libc rand()/srand(). These match the AVR core's behaviour
// and integer-truncation semantics so stock sketches port unchanged.

#include <stdlib.h>     // rand()/srand() — include BEFORE Arduino.h, whose
                        // abs()/round() macros would otherwise mangle its decls
#include "Arduino.h"

// map() picks up C linkage from Arduino.h's `extern "C"` block (its declaration
// is visible here); random()/randomSeed() are C++ (overloaded).

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

long random(long howbig)
{
    if (howbig == 0) return 0;
    return rand() % howbig;
}

long random(long howsmall, long howbig)
{
    if (howsmall >= howbig) return howsmall;
    long diff = howbig - howsmall;
    return random(diff) + howsmall;
}

void randomSeed(unsigned long seed)
{
    if (seed != 0) srand(seed);
}
