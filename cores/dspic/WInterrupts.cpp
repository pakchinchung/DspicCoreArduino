// attachInterrupt() / detachInterrupt() for dsPIC33CK, via Change Notification.
//
// WHY CN (not the INT0..INT3 external-interrupt pins): INT1/2/3 need a PPS *input*
// mapping, and PPS only covers ports B/C/D on this device — so INTx physically
// cannot reach RA/RE pins (incl. the DM330030 buttons on RE7/8/9). Change
// Notification works on ANY pin and, in edge-detect style (CNCONx.CNSTYLE=1),
// gives per-pin edge selection: CNEN0x = rising-edge enable, CNEN1x = falling-edge
// enable (both = CHANGE), CNFx = per-pin "this pin fired" flag. One interrupt per
// port (_CNAInterrupt.._CNEInterrupt) scans CNFx and dispatches the callbacks.
//
// Pin ids are native-name (port<<4)|bit (see dspic_pins.h). Ports A..E = 0..4.

#include "Arduino.h"

#if defined(__dsPIC33C__)
#include <xc.h>

#define CN_PORTS 5   // A..E (the ports this device implements)

// Per-port CN register pointers (module-uniform layout, reached by address).
static volatile uint16_t *const CN_CON[CN_PORTS] = { &CNCONA, &CNCONB, &CNCONC, &CNCOND, &CNCONE };
static volatile uint16_t *const CN_EN0[CN_PORTS] = { &CNEN0A, &CNEN0B, &CNEN0C, &CNEN0D, &CNEN0E };
static volatile uint16_t *const CN_EN1[CN_PORTS] = { &CNEN1A, &CNEN1B, &CNEN1C, &CNEN1D, &CNEN1E };
static volatile uint16_t *const CN_F[CN_PORTS]   = { &CNFA,   &CNFB,   &CNFC,   &CNFD,   &CNFE   };

#define CNCON_ON       0x8000   // CNCONx.ON  (bit 15) — module enable
#define CNCON_CNSTYLE  0x0800   // CNCONx.CNSTYLE (bit 11) — 1 = edge-detect style

// User callbacks, indexed [port][bit]. Static => zero-initialised (no handler).
static voidFuncPtr cn_cb[CN_PORTS][16];

// Per-port interrupt-enable / flag-clear (the bit accessors are individual
// #defines, so a switch is the only way to reach them by index).
static void cn_irq_enable(uint8_t port)
{
    switch (port) {
    case 0: _CNAIF = 0; _CNAIP = 4; _CNAIE = 1; break;
    case 1: _CNBIF = 0; _CNBIP = 4; _CNBIE = 1; break;
    case 2: _CNCIF = 0; _CNCIP = 4; _CNCIE = 1; break;
    case 3: _CNDIF = 0; _CNDIP = 4; _CNDIE = 1; break;
    case 4: _CNEIF = 0; _CNEIP = 4; _CNEIE = 1; break;
    default: break;
    }
}

static void cn_irq_disable(uint8_t port)
{
    switch (port) {
    case 0: _CNAIE = 0; break;
    case 1: _CNBIE = 0; break;
    case 2: _CNCIE = 0; break;
    case 3: _CNDIE = 0; break;
    case 4: _CNEIE = 0; break;
    default: break;
    }
}

void attachInterrupt(uint8_t pin, voidFuncPtr callback, int mode)
{
    uint8_t port = PIN_PORT(pin);
    uint8_t bit  = PIN_BIT(pin);
    if (port >= CN_PORTS || !callback) return;
    if (port >= NUM_PORTS || !g_ports[port].tris) return;   // port not on device

    uint16_t m = (uint16_t)(1u << bit);

    // Make the pin a digital input so CN can observe it. Leave the pull-up as the
    // user left it via pinMode(); the DM330030 buttons have external pull-ups.
    if (g_ports[port].ansel) *g_ports[port].ansel &= ~m;
    *g_ports[port].tris |= m;

    cn_cb[port][bit] = callback;

    // Edge select: RISING -> CNEN0, FALLING -> CNEN1, CHANGE -> both.
    if (mode == FALLING) { *CN_EN0[port] &= ~m; *CN_EN1[port] |=  m; }
    else if (mode == RISING) { *CN_EN0[port] |=  m; *CN_EN1[port] &= ~m; }
    else /* CHANGE (and any unsupported level mode) */ { *CN_EN0[port] |= m; *CN_EN1[port] |= m; }

    *CN_F[port] &= ~m;                          // clear any stale flag for this pin
    *CN_CON[port] = CNCON_ON | CNCON_CNSTYLE;   // edge-detect style, module on
    cn_irq_enable(port);
}

void detachInterrupt(uint8_t pin)
{
    uint8_t port = PIN_PORT(pin);
    uint8_t bit  = PIN_BIT(pin);
    if (port >= CN_PORTS) return;

    uint16_t m = (uint16_t)(1u << bit);
    *CN_EN0[port] &= ~m;
    *CN_EN1[port] &= ~m;
    *CN_F[port]   &= ~m;
    cn_cb[port][bit] = 0;

    // If no pins on this port still want edges, switch the port's interrupt off.
    if (*CN_EN0[port] == 0 && *CN_EN1[port] == 0) cn_irq_disable(port);
}

// Walk a port's fired flags, call the registered handlers, clear what we handled.
static inline void cn_dispatch(uint8_t port)
{
    uint16_t flags = *CN_F[port];
    *CN_F[port] = (uint16_t)(*CN_F[port] & ~flags);   // clear handled edges
    for (uint8_t b = 0; flags; b++, flags >>= 1) {
        if ((flags & 1u) && cn_cb[port][b]) cn_cb[port][b]();
    }
}

// One ISR per port; each clears its own IFS flag after dispatching.
extern "C" void __attribute__((interrupt, no_auto_psv)) _CNAInterrupt(void) { cn_dispatch(0); _CNAIF = 0; }
extern "C" void __attribute__((interrupt, no_auto_psv)) _CNBInterrupt(void) { cn_dispatch(1); _CNBIF = 0; }
extern "C" void __attribute__((interrupt, no_auto_psv)) _CNCInterrupt(void) { cn_dispatch(2); _CNCIF = 0; }
extern "C" void __attribute__((interrupt, no_auto_psv)) _CNDInterrupt(void) { cn_dispatch(3); _CNDIF = 0; }
extern "C" void __attribute__((interrupt, no_auto_psv)) _CNEInterrupt(void) { cn_dispatch(4); _CNEIF = 0; }

#elif defined(__dsPIC33A__)
// dsPIC33AK Change Notification — same edge-detect design as CK, but this device
// has ports A..D only and 32-bit CN SFRs. Vectors are _CNAInterrupt.._CNDInterrupt
// (named-vector convention works on AK's relocatable IVT, same as _T1Interrupt).
#include <xc.h>

#define CN_PORTS 4   // A..D

static volatile uint32_t *const CN_CON[CN_PORTS] = { &CNCONA, &CNCONB, &CNCONC, &CNCOND };
static volatile uint32_t *const CN_EN0[CN_PORTS] = { &CNEN0A, &CNEN0B, &CNEN0C, &CNEN0D };
static volatile uint32_t *const CN_EN1[CN_PORTS] = { &CNEN1A, &CNEN1B, &CNEN1C, &CNEN1D };
static volatile uint32_t *const CN_F[CN_PORTS]   = { &CNFA,   &CNFB,   &CNFC,   &CNFD   };

#define CNCON_ON       0x8000u   // CNCONx.ON (bit 15)
#define CNCON_CNSTYLE  0x0800u   // CNCONx.CNSTYLE (bit 11) — edge-detect style

static voidFuncPtr cn_cb[CN_PORTS][16];

static void cn_irq_enable(uint8_t port)
{
    switch (port) {
    case 0: _CNAIF = 0; _CNAIP = 4; _CNAIE = 1; break;
    case 1: _CNBIF = 0; _CNBIP = 4; _CNBIE = 1; break;
    case 2: _CNCIF = 0; _CNCIP = 4; _CNCIE = 1; break;
    case 3: _CNDIF = 0; _CNDIP = 4; _CNDIE = 1; break;
    default: break;
    }
}

static void cn_irq_disable(uint8_t port)
{
    switch (port) {
    case 0: _CNAIE = 0; break;
    case 1: _CNBIE = 0; break;
    case 2: _CNCIE = 0; break;
    case 3: _CNDIE = 0; break;
    default: break;
    }
}

void attachInterrupt(uint8_t pin, voidFuncPtr callback, int mode)
{
    uint8_t port = PIN_PORT(pin);
    uint8_t bit  = PIN_BIT(pin);
    if (port >= CN_PORTS || !callback) return;
    if (port >= NUM_PORTS || !g_ports[port].tris) return;

    uint32_t m = (uint32_t)(1u << bit);

    if (g_ports[port].ansel) *g_ports[port].ansel &= ~m;
    *g_ports[port].tris |= m;                   // pin = digital input for CN

    cn_cb[port][bit] = callback;

    if (mode == FALLING)     { *CN_EN0[port] &= ~m; *CN_EN1[port] |=  m; }
    else if (mode == RISING) { *CN_EN0[port] |=  m; *CN_EN1[port] &= ~m; }
    else /* CHANGE */        { *CN_EN0[port] |=  m; *CN_EN1[port] |=  m; }

    *CN_F[port] &= ~m;                          // clear stale flag
    *CN_CON[port] = CNCON_ON | CNCON_CNSTYLE;   // edge-detect style, module on
    cn_irq_enable(port);
}

void detachInterrupt(uint8_t pin)
{
    uint8_t port = PIN_PORT(pin);
    uint8_t bit  = PIN_BIT(pin);
    if (port >= CN_PORTS) return;

    uint32_t m = (uint32_t)(1u << bit);
    *CN_EN0[port] &= ~m;
    *CN_EN1[port] &= ~m;
    *CN_F[port]   &= ~m;
    cn_cb[port][bit] = 0;

    if (*CN_EN0[port] == 0 && *CN_EN1[port] == 0) cn_irq_disable(port);
}

static inline void cn_dispatch(uint8_t port)
{
    uint32_t flags = *CN_F[port];
    *CN_F[port] = (*CN_F[port] & ~flags);       // clear handled edges
    for (uint8_t b = 0; flags; b++, flags >>= 1) {
        if ((flags & 1u) && cn_cb[port][b]) cn_cb[port][b]();
    }
}

extern "C" void __attribute__((interrupt, no_auto_psv)) _CNAInterrupt(void) { cn_dispatch(0); _CNAIF = 0; }
extern "C" void __attribute__((interrupt, no_auto_psv)) _CNBInterrupt(void) { cn_dispatch(1); _CNBIF = 0; }
extern "C" void __attribute__((interrupt, no_auto_psv)) _CNCInterrupt(void) { cn_dispatch(2); _CNCIF = 0; }
extern "C" void __attribute__((interrupt, no_auto_psv)) _CNDInterrupt(void) { cn_dispatch(3); _CNDIF = 0; }

#else  // ---- non-dsPIC33C: compile-safe stub ---------------------------------

void attachInterrupt(uint8_t, voidFuncPtr, int) { }
void detachInterrupt(uint8_t) { }

#endif
