/*
 * dspic_pins.h — pin-by-native-name model for dspicArduino (CK + AK).
 *
 * A pin id encodes its port and bit: (port << 4) | bit, with ports numbered
 * A=0, B=1, C=2, D=3, E=4, F=5, G=6. So RE6 == (4<<4)|6 == 0x46. This lets a
 * sketch refer to any pin by its datasheet name — `#define LED_USER RE6` — and
 * lets custom boards work without enumerating an Arduino pin map.
 *
 * pinMode/digitalWrite/digitalRead (wiring_digital.cpp) decode the id and look
 * up the port's SFRs in g_ports[], a per-port register table that each board
 * variant defines (the TRISx/LATx/... names come from <xc.h> and only exist
 * for ports the device actually has). Variants may ALSO add integer aliases
 * (D0, D1, ..., LED_BUILTIN) on top of these names for stock-sketch portability.
 */
#ifndef DSPIC_PINS_H
#define DSPIC_PINS_H

#include <stdint.h>

/* ---- pin id encoding ----------------------------------------------------- *
 * Port indices are passed as literals (A=0..G=6) below rather than named
 * constants — names like PC would collide with device SFRs (dsPIC33A has a
 * `PC` register).                                                             */
#define DSPIC_PIN(port, bit)  ((uint16_t)(((port) << 4) | ((bit) & 0x0F)))
#define PIN_PORT(p)           ((uint8_t)((p) >> 4))
#define PIN_BIT(p)            ((uint8_t)((p) & 0x0F))
#define DSPIC_PIN_NONE        ((uint16_t)0xFFFF)   /* "no pin" sentinel */

/* ---- every native pin name, RA0..RG15 (pure encoding, device-agnostic) ---- *
 * A name existing here does NOT mean the device has that pin; pinMode() &c.
 * validate against the variant's g_ports[] table at runtime.                  */
#define _DSPIC_PORT_PINS(L, P)                 \
    enum {                                     \
        R##L##0  = DSPIC_PIN(P,0),  R##L##1  = DSPIC_PIN(P,1),  \
        R##L##2  = DSPIC_PIN(P,2),  R##L##3  = DSPIC_PIN(P,3),  \
        R##L##4  = DSPIC_PIN(P,4),  R##L##5  = DSPIC_PIN(P,5),  \
        R##L##6  = DSPIC_PIN(P,6),  R##L##7  = DSPIC_PIN(P,7),  \
        R##L##8  = DSPIC_PIN(P,8),  R##L##9  = DSPIC_PIN(P,9),  \
        R##L##10 = DSPIC_PIN(P,10), R##L##11 = DSPIC_PIN(P,11), \
        R##L##12 = DSPIC_PIN(P,12), R##L##13 = DSPIC_PIN(P,13), \
        R##L##14 = DSPIC_PIN(P,14), R##L##15 = DSPIC_PIN(P,15)  \
    }
_DSPIC_PORT_PINS(A, 0);
_DSPIC_PORT_PINS(B, 1);
_DSPIC_PORT_PINS(C, 2);
_DSPIC_PORT_PINS(D, 3);
_DSPIC_PORT_PINS(E, 4);
_DSPIC_PORT_PINS(F, 5);
_DSPIC_PORT_PINS(G, 6);

/* ---- SFR width -----------------------------------------------------------*
 * dsPIC33A (AK) GPIO SFRs are 32-bit; dsPIC33C (CK) and the classic 16-bit
 * parts are 16-bit. The port table and bit masks must match, or the access
 * width is wrong / the pointer types mismatch <xc.h>.                         */
#if defined(__dsPIC33A__) || defined(__HAS_ISA32__)
typedef uint32_t dspic_sfr_t;
#else
typedef uint16_t dspic_sfr_t;
#endif

/* ---- per-port register table -------------------------------------------- *
 * One entry per port index. A variant leaves tris == NULL for ports the
 * device does not implement; cnpu == NULL when the port has no weak pull-up.  */
typedef struct {
    volatile dspic_sfr_t *tris;   /* TRISx  — direction (1=input, 0=output) */
    volatile dspic_sfr_t *lat;    /* LATx   — output latch                  */
    volatile dspic_sfr_t *port;   /* PORTx  — input read                    */
    volatile dspic_sfr_t *ansel;  /* ANSELx — analog/digital (NULL if none) */
    volatile dspic_sfr_t *cnpu;   /* CNPUx  — weak pull-up   (NULL if none) */
} PortReg;

#ifdef __cplusplus
extern "C" {
#endif
extern const PortReg  g_ports[];   /* defined in variants/<board>/variant.cpp */
extern const uint8_t  NUM_PORTS;   /* number of valid entries in g_ports[]    */
#ifdef __cplusplus
}
#endif

#endif /* DSPIC_PINS_H */
