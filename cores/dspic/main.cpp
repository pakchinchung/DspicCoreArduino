#include "Arduino.h"

// C++ ABI stub: the linker needs this for the abstract Print/Stream bases
// (their vtables reference it). It's only reached if a pure-virtual is called,
// which shouldn't happen — hang so it's catchable rather than running wild.
extern "C" void __cxa_pure_virtual(void) { for (;;) {} }

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
