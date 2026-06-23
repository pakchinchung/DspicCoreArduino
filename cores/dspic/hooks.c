// Weak default implementations of setup() and loop() so the linker
// never complains if the user somehow omits one. In practice the
// Arduino IDE always provides both from the sketch.
#include <stdint.h>

__attribute__((weak)) void setup(void) {}
__attribute__((weak)) void loop(void)  {}
