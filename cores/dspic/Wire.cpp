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

#elif defined(__dsPIC33A__)
// ---- dsPIC33AK I2C master (I2CxCON1/STAT1, split HBRG/LBRG; 32-bit) ----------
// Same master bit-bang sequence as CK, but enable bit is ON (not I2CEN), baud is
// split into I2CxHBRG/I2CxLBRG, and registers are 32-bit. I2C is not PPS-routed;
// the alternate I2C2 pins (ASDA2=RD8 / ASCL2=RD7) are selected by FDEVOPT_ALTI2C2
// (set ON in variant.cpp). The Curiosity LCD is on I2C2 = Wire1.
#include <xc.h>

#define I2C_FCY (F_CPU / 2UL)

// AK has only I2C1 and I2C2 (no I2C3) — module 3 (Wire2) aliases I2C2.
static volatile uint32_t *ak_icon1(uint8_t m){ return m==1?&I2C1CON1 : &I2C2CON1; }
static volatile uint32_t *ak_istat(uint8_t m){ return m==1?&I2C1STAT1: &I2C2STAT1;}
static volatile uint32_t *ak_itrn (uint8_t m){ return m==1?&I2C1TRN  : &I2C2TRN; }
static volatile uint32_t *ak_ircv (uint8_t m){ return m==1?&I2C1RCV  : &I2C2RCV; }
static volatile uint32_t *ak_ihbrg(uint8_t m){ return m==1?&I2C1HBRG : &I2C2HBRG; }
static volatile uint32_t *ak_ilbrg(uint8_t m){ return m==1?&I2C1LBRG : &I2C2LBRG; }
#define ICON(m)  (*(volatile I2C1CON1BITS  *)ak_icon1(m))
#define ISTAT(m) (*(volatile I2C1STAT1BITS *)ak_istat(m))

#define I2C_GUARD 200000u
#define WAIT_WHILE(expr) do { uint32_t _g = I2C_GUARD; while ((expr) && --_g) {} } while (0)

void TwoWire::setbrg(uint32_t fscl)
{
    // SCL high+low halves. brg ~= I2Cclk/(2*FSCL). Min 4 (datasheet). Exact freq
    // isn't critical for a scanner/LCD; this lands ~8-100 kHz across clock options.
    int32_t brg = (int32_t)(I2C_FCY / (2UL * (fscl ? fscl : 100000UL)));
    if (brg < 4) brg = 4;
    if (brg > 0xFFFFFF) brg = 0xFFFFFF;
    *ak_ihbrg(_module) = (uint32_t)brg;
    *ak_ilbrg(_module) = (uint32_t)brg;
}

void TwoWire::pinSetup() { /* AK I2C alt pins (RD7/RD8) are not analog -> nothing */ }

void TwoWire::start()  { ICON(_module).SEN  = 1; WAIT_WHILE(ICON(_module).SEN);  }
void TwoWire::rstart() { ICON(_module).RSEN = 1; WAIT_WHILE(ICON(_module).RSEN); }
void TwoWire::stop()   { ICON(_module).PEN  = 1; WAIT_WHILE(ICON(_module).PEN);  }

bool TwoWire::tx(uint8_t b)
{
    *ak_itrn(_module) = b;
    WAIT_WHILE(ISTAT(_module).TRSTAT);          // wait until byte sent
    return (ISTAT(_module).ACKSTAT == 0);       // 0 = ACK
}

uint8_t TwoWire::rx(bool ack)
{
    ICON(_module).RCEN = 1;
    WAIT_WHILE(ICON(_module).RCEN);
    uint8_t b = (uint8_t)*ak_ircv(_module);
    ICON(_module).ACKDT = ack ? 0 : 1;          // 0 = ACK, 1 = NACK
    ICON(_module).ACKEN = 1;
    WAIT_WHILE(ICON(_module).ACKEN);
    return b;
}

void TwoWire::begin()
{
    if (_module == 1) _I2C1MD = 0;              // enable module clock (PMD);
    else              _I2C2MD = 0;              // AK has only I2C1/I2C2
    __asm__ volatile ("nop"); __asm__ volatile ("nop");
    pinSetup();
    *ak_icon1(_module) = 0;
    setbrg(_clock);
    ICON(_module).ON = 1;                        // enable (takes over the pins)
}

void TwoWire::begin(uint8_t sclPin, uint8_t sdaPin) { (void)sclPin; (void)sdaPin; begin(); }

void TwoWire::setClock(uint32_t hz)
{
    _clock = hz;
    ICON(_module).ON = 0;
    setbrg(hz);
    ICON(_module).ON = 1;
}

void TwoWire::beginTransmission(uint8_t address) { _txAddr = address; _txLen = 0; }

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
    if (!tx((uint8_t)(_txAddr << 1))) { stop(); return 2; }
    for (uint8_t i = 0; i < _txLen; i++)
        if (!tx(_txBuf[i])) { stop(); return 3; }
    if (sendStop) stop();
    return 0;
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity, bool sendStop)
{
    if (quantity > WIRE_BUFFER_SIZE) quantity = WIRE_BUFFER_SIZE;
    _rxLen = 0; _rxIdx = 0;
    start();
    if (!tx((uint8_t)((address << 1) | 1))) { stop(); return 0; }
    for (uint8_t i = 0; i < quantity; i++) {
        bool last = (i == (uint8_t)(quantity - 1));
        _rxBuf[_rxLen++] = rx(!last);
    }
    if (sendStop) stop();
    return _rxLen;
}

int TwoWire::available() { return _rxLen - _rxIdx; }
int TwoWire::read()      { return (_rxIdx < _rxLen) ? _rxBuf[_rxIdx++] : -1; }
int TwoWire::peek()      { return (_rxIdx < _rxLen) ? _rxBuf[_rxIdx]   : -1; }

TwoWire Wire (0, 0, 0, 0, 0, 1);   // I2C1
TwoWire Wire1(0, 0, 0, 0, 0, 2);   // I2C2 (Curiosity LCD on alt pins ASDA2/ASCL2)
TwoWire Wire2(0, 0, 0, 0, 0, 3);   // I2C3

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
