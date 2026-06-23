// pinMode(), digitalWrite(), digitalRead()
//
// Pins are native-name ids encoded as (port<<4)|bit (see dspic_pins.h). We
// decode the id, look the port up in the variant's g_ports[] table, and touch
// the SFRs directly. A pin on a port the device doesn't implement (table entry
// with tris == NULL, or port index >= NUM_PORTS) is silently ignored.

#include "Arduino.h"

// Resolve a pin id to its port-register set, or NULL if the pin is invalid.
static inline const PortReg *resolve(uint8_t pin)
{
    uint8_t port = PIN_PORT(pin);
    if (port >= NUM_PORTS) return 0;
    const PortReg *p = &g_ports[port];
    if (!p->tris) return 0;                 // port not present on this device
    return p;
}

#define PIN_MASK(pin) ((dspic_sfr_t)1 << PIN_BIT(pin))

void pinMode(uint8_t pin, uint8_t mode)
{
    const PortReg *p = resolve(pin);
    if (!p) return;
    dspic_sfr_t m = PIN_MASK(pin);

    if (p->ansel) *p->ansel &= ~m;          // always digital for digital I/O

    if (mode == OUTPUT) {
        *p->tris &= ~m;                     // TRISx bit = 0 -> output
    } else {
        *p->tris |= m;                      // TRISx bit = 1 -> input
        // Weak pull-up only when the port supports CNPUx and INPUT_PULLUP asked.
        if (p->cnpu) {
            if (mode == INPUT_PULLUP) *p->cnpu |= m;
            else                      *p->cnpu &= ~m;
        }
    }
}

void digitalWrite(uint8_t pin, uint8_t val)
{
    const PortReg *p = resolve(pin);
    if (!p) return;
    dspic_sfr_t m = PIN_MASK(pin);

    if (val == LOW) *p->lat &= ~m;
    else            *p->lat |=  m;
}

int digitalRead(uint8_t pin)
{
    const PortReg *p = resolve(pin);
    if (!p) return LOW;

    return (*p->port & PIN_MASK(pin)) ? HIGH : LOW;
}
