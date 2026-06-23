#include "Arduino.h"

// Board-level hardware init (clock, Timer1 for millis, etc.).
// Defined in variants/<board>/variant.cpp as extern "C".
extern "C" void initVariant(void);

// Called by the XC-DSC C runtime after .data/.bss init and
// static C++ constructors have run. This is our Arduino entry point.
int main(void)
{
    initVariant();

    setup();

    for (;;) {
        loop();
    }

    return 0;
}
