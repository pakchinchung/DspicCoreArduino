// Arduino Wire (I2C) master for dsPIC33CK, multi-instance (I2C1/2/3).
// Non-CK builds get a compile-safe stub.
#include "Wire.h"

// Construct on a module regardless of family (so the globals exist everywhere).
TwoWire::TwoWire(volatile uint16_t *conl, volatile uint16_t *brg,
                 volatile uint16_t *trn,  volatile uint16_t *rcv,
                 volatile uint16_t *stat, uint8_t moduleId)
    : _conl(conl), _brg(brg), _trn(trn), _rcv(rcv), _stat(stat), _module(moduleId)
{
}

#if defined(__dsPIC33C__)
#include "xc_compat.h"

#define I2C_FCY (F_CPU / 2UL)              // 50 MHz here

// All I2Cx modules share the bit layout of module 1, so we reach the control
// and status bits through the stored register pointer cast to module-1's
// bitfield struct types (one code path, named fields, no per-module switch).
#define CONLB (*(volatile I2C1CONLBITS *)_conl)
#define STATB (*(volatile I2C1STATBITS *)_stat)

// All waits are bounded so a dead/unwired bus reports failure instead of hanging.
#define I2C_GUARD 20000u
#define WAIT_WHILE(expr) do { uint16_t _g = I2C_GUARD; while ((expr) && --_g) {} } while (0)

void TwoWire::setbrg(uint32_t fscl)
{
    // dsPIC: I2CxBRG = (FCY/FSCL - FCY/10000000) - 1
    int32_t brg = (int32_t)(I2C_FCY / fscl) - (int32_t)(I2C_FCY / 10000000UL) - 1;
    if (brg < 2) brg = 2;
    if (brg > 65535) brg = 65535;
    *_brg = (uint16_t)brg;
}

// Clear ANSEL (analog) on the active SDA/SCL pins so the digital I2C input
// works. Only pins that actually have an analog function need this; the rest
// have no ANSELx bit (writing one would not compile), so they are skipped.
void TwoWire::pinSetup()
{
    switch (_module) {
    case 1:
#if defined(DSPIC_I2C1_PRIMARY)
        ANSELBbits.ANSELB8 = 0;            // SCL1 = RB8
        ANSELBbits.ANSELB9 = 0;            // SDA1 = RB9
#else
        /* alternate RC8 (SDA) / RC9 (SCL): no analog on those pins */
#endif
        break;
    case 2:
        /* SDA2 = RB5 / SCL2 = RB6: no analog on those pins */
        break;
    case 3:
        ANSELBbits.ANSELB7 = 0;            // SDA3 = RB7
        ANSELBbits.ANSELB2 = 0;            // SCL3 = RB2
        break;
    default: break;
    }
}

void TwoWire::start()  { CONLB.SEN  = 1; WAIT_WHILE(CONLB.SEN);  }
void TwoWire::rstart() { CONLB.RSEN = 1; WAIT_WHILE(CONLB.RSEN); }
void TwoWire::stop()   { CONLB.PEN  = 1; WAIT_WHILE(CONLB.PEN);  }

bool TwoWire::tx(uint8_t b)                  // returns true if ACKed
{
    *_trn = b;
    WAIT_WHILE(STATB.TRSTAT);                // wait until byte sent
    return (STATB.ACKSTAT == 0);            // 0 = ACK
}

uint8_t TwoWire::rx(bool ack)
{
    CONLB.RCEN = 1;
    WAIT_WHILE(CONLB.RCEN);                  // wait for a received byte
    uint8_t b = (uint8_t)(*_rcv);
    CONLB.ACKDT = ack ? 0 : 1;               // 0 = ACK, 1 = NACK
    CONLB.ACKEN = 1;
    WAIT_WHILE(CONLB.ACKEN);
    return b;
}

void TwoWire::begin()
{
    pinSetup();
    *_conl = 0;
    setbrg(_clock);
    CONLB.I2CEN = 1;                         // enable module (takes over the pins)
}

// dsPIC convenience: standard Arduino Wire.begin() takes no pins, but accepting
// (scl,sda) lets sketches read naturally, e.g. Wire.begin(RC9, RC8). I2C is not
// PPS-routed, so the pins don't remap anything at runtime — the primary/alt
// choice is the compile-time ALTI2Cx bit. We just start the module.
void TwoWire::begin(uint8_t sclPin, uint8_t sdaPin)
{
    (void)sclPin; (void)sdaPin;
    begin();
}

void TwoWire::setClock(uint32_t hz)
{
    _clock = hz;
    CONLB.I2CEN = 0;
    setbrg(hz);
    CONLB.I2CEN = 1;
}

void TwoWire::beginTransmission(uint8_t address)
{
    _txAddr = address;
    _txLen = 0;
}

size_t TwoWire::write(uint8_t data)
{
    if (_txLen < WIRE_BUFFER_SIZE) { _txBuf[_txLen++] = data; return 1; }
    return 0;
}

size_t TwoWire::write(const uint8_t *data, size_t n)
{
    size_t w = 0;
    while (n-- && _txLen < WIRE_BUFFER_SIZE) { _txBuf[_txLen++] = *data++; w++; }
    return w;
}

uint8_t TwoWire::endTransmission(bool sendStop)
{
    start();
    if (!tx((uint8_t)(_txAddr << 1))) {        // address + write
        stop();
        return 2;                              // NACK on address
    }
    for (uint8_t i = 0; i < _txLen; i++) {
        if (!tx(_txBuf[i])) { stop(); return 3; }   // NACK on data
    }
    if (sendStop) stop();
    return 0;
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity, bool sendStop)
{
    if (quantity > WIRE_BUFFER_SIZE) quantity = WIRE_BUFFER_SIZE;
    _rxLen = 0; _rxIdx = 0;

    start();
    if (!tx((uint8_t)((address << 1) | 1))) {  // address + read
        stop();
        return 0;
    }
    for (uint8_t i = 0; i < quantity; i++) {
        bool last = (i == (uint8_t)(quantity - 1));
        _rxBuf[_rxLen++] = rx(!last);          // ACK all but the last byte
    }
    if (sendStop) stop();
    return _rxLen;
}

int TwoWire::available() { return _rxLen - _rxIdx; }
int TwoWire::read()      { return (_rxIdx < _rxLen) ? _rxBuf[_rxIdx++] : -1; }
int TwoWire::peek()      { return (_rxIdx < _rxLen) ? _rxBuf[_rxIdx]   : -1; }

// ---- the three module instances ----
TwoWire Wire (&I2C1CONL, &I2C1BRG, &I2C1TRN, &I2C1RCV, &I2C1STAT, 1);
TwoWire Wire1(&I2C2CONL, &I2C2BRG, &I2C2TRN, &I2C2RCV, &I2C2STAT, 2);
TwoWire Wire2(&I2C3CONL, &I2C3BRG, &I2C3TRN, &I2C3RCV, &I2C3STAT, 3);

#else  // ---- non-dsPIC33C stub ----
void    TwoWire::setbrg(uint32_t) {}
void    TwoWire::pinSetup() {}
void    TwoWire::start() {}
void    TwoWire::rstart() {}
void    TwoWire::stop() {}
bool    TwoWire::tx(uint8_t) { return false; }
uint8_t TwoWire::rx(bool) { return 0; }
void    TwoWire::begin() {}
void    TwoWire::begin(uint8_t, uint8_t) {}
void    TwoWire::setClock(uint32_t) {}
void    TwoWire::beginTransmission(uint8_t) {}
size_t  TwoWire::write(uint8_t) { return 0; }
size_t  TwoWire::write(const uint8_t *, size_t) { return 0; }
uint8_t TwoWire::endTransmission(bool) { return 4; }
uint8_t TwoWire::requestFrom(uint8_t, uint8_t, bool) { return 0; }
int     TwoWire::available() { return 0; }
int     TwoWire::read() { return -1; }
int     TwoWire::peek() { return -1; }

TwoWire Wire (0, 0, 0, 0, 0, 1);
TwoWire Wire1(0, 0, 0, 0, 0, 2);
TwoWire Wire2(0, 0, 0, 0, 0, 3);
#endif
