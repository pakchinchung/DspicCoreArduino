// HardwareSerial — dsPIC33CK "new" UART, multi-instance (UART1/2/3).
// dsPIC33C only; other families get a compile-safe stub.
//
//   Serial  -> UART1 (PKoB4 virtual COM: U1TX=RP68/RD4, U1RX=RP67/RD3)
//   Serial1 -> UART2     Serial2 -> UART3   (set pins with setPins() first)

#include "HardwareSerial.h"
#include "Arduino.h"
#include <xc.h>

HardwareSerial::HardwareSerial(volatile uint16_t *mode,  volatile uint16_t *modeH,
                               volatile uint16_t *staH,  volatile uint16_t *brg,
                               volatile uint16_t *rxreg, volatile uint16_t *txreg,
                               uint8_t moduleId)
    : _mode(mode), _modeH(modeH), _staH(staH), _brg(brg),
      _rxreg(rxreg), _txreg(txreg), _module(moduleId)
{
}

#if defined(__dsPIC33C__) || defined(__dsPIC33A__)
// PPS pin name->RPn and pad setup come from dspic_pins.h (shared with SPI):
//   dspic_pin_to_rp(pin) / dspic_pin_pps_cfg(pin, output)

void HardwareSerial::setPins(uint8_t txPin, uint8_t rxPin)
{
    _txPin = txPin; _rxPin = rxPin;          // native pin names; applied in begin()
}

void HardwareSerial::begin(unsigned long baud, uint8_t txPin, uint8_t rxPin)
{
    _txPin = txPin; _rxPin = rxPin;
    begin(baud);
}
#endif

#if defined(__dsPIC33C__)

// Baud generator runs from FP = FCY = F_CPU/2 (matches wiring.cpp Timer1).
#ifndef F_CPU
#define F_CPU 100000000UL
#endif
#define UART_FCY  (F_CPU / 2UL)

// All UARTx share module 1's bit layout; reach bits through the stored pointer
// cast to module-1's bitfield struct types.
#define MODEB  (*(volatile U1MODEBITS  *)_mode)
#define MODEHB (*(volatile U1MODEHBITS *)_modeH)
#define STAHB  (*(volatile U1STAHBITS  *)_staH)

// Map a remappable pin's output function (RPORx: contiguous words, two 6-bit
// fields each; RPn 32 = RPOR0 low byte). Module-independent.
static void rpor_set(uint8_t rp, uint8_t fn)
{
    volatile uint16_t *rpor = (&RPOR0) + ((rp - 32) >> 1);
    uint8_t sh = ((rp - 32) & 1) ? 8 : 0;
    *rpor = (uint16_t)((*rpor & ~(0x3Fu << sh)) | ((uint16_t)(fn & 0x3F) << sh));
}

void HardwareSerial::begin(unsigned long baud)
{
    // Resolve native pin names -> RPn and make the pins digital + directioned.
    uint8_t txRP = dspic_pin_to_rp(_txPin), rxRP = dspic_pin_to_rp(_rxPin);
    dspic_pin_pps_cfg(_txPin, 1);                // TX = output
    dspic_pin_pps_cfg(_rxPin, 0);                // RX = input

    // --- PPS: route TX (output func) and RX (input select) per module --------
    __builtin_write_RPCON(0x0000);               // unlock PPS
    if (txRP != 0xFF) switch (_module) {          // U1/2/3 TX func = 1/3/27
    case 1: rpor_set(txRP, 1);  break;
    case 2: rpor_set(txRP, 3);  break;
    case 3: rpor_set(txRP, 27); break;
    default: break;
    }
    if (rxRP != 0xFF) switch (_module) {
    case 1: _U1RXR = rxRP; break;
    case 2: _U2RXR = rxRP; break;
    case 3: _U3RXR = rxRP; break;
    default: break;
    }
    __builtin_write_RPCON(0x0800);               // lock PPS

    // --- 8 data bits, no parity, 1 stop; BRGH=0 (÷16); baud clock = FP --------
    *_mode = 0;
    MODEB.MOD      = 0;
    MODEB.BRGH     = 0;
    MODEHB.BCLKSEL = 0;
    *_brg = (uint16_t)((UART_FCY / (16UL * baud)) - 1UL);

    MODEB.UARTEN = 1;
    MODEB.UTXEN  = 1;
    MODEB.URXEN  = 1;

    // --- RX interrupt enable (per-module bit accessors) ----------------------
    _head = _tail = 0;
    switch (_module) {
    case 1: _U1RXIF = 0; _U1RXIP = 4; _U1RXIE = 1; break;
    case 2: _U2RXIF = 0; _U2RXIP = 4; _U2RXIE = 1; break;
    case 3: _U3RXIF = 0; _U3RXIP = 4; _U3RXIE = 1; break;
    default: break;
    }
}

void HardwareSerial::end(void)
{
    switch (_module) {
    case 1: _U1RXIE = 0; break;
    case 2: _U2RXIE = 0; break;
    case 3: _U3RXIE = 0; break;
    default: break;
    }
    MODEB.UARTEN = 0;
}

int HardwareSerial::available(void)
{
    return (uint8_t)(_head - _tail) & SERIAL_RX_MASK;
}

int HardwareSerial::peek(void)
{
    if (_head == _tail) return -1;
    return _rx[_tail];
}

int HardwareSerial::read(void)
{
    if (_head == _tail) return -1;
    uint8_t c = _rx[_tail];
    _tail = (_tail + 1) & SERIAL_RX_MASK;
    return c;
}

size_t HardwareSerial::write(uint8_t c)
{
    while (STAHB.UTXBF) { }                      // wait while TX FIFO full
    *_txreg = c;
    return 1;
}

void HardwareSerial::rxHandler(void)
{
    while (!STAHB.URXBE) {                        // while RX FIFO not empty
        uint8_t c = (uint8_t)(*_rxreg);
        uint8_t next = (_head + 1) & SERIAL_RX_MASK;
        if (next != _tail) {                      // drop on overflow
            _rx[_head] = c;
            _head = next;
        }
    }
}

// ---- the three module instances ----
HardwareSerial Serial (&U1MODE, &U1MODEH, &U1STAH, &U1BRG, &U1RXREG, &U1TXREG, 1);
HardwareSerial Serial1(&U2MODE, &U2MODEH, &U2STAH, &U2BRG, &U2RXREG, &U2TXREG, 2);
HardwareSerial Serial2(&U3MODE, &U3MODEH, &U3STAH, &U3BRG, &U3RXREG, &U3TXREG, 3);

// ---- one RX ISR per module, each draining its instance's FIFO --------------
extern "C" void __attribute__((interrupt, no_auto_psv)) _U1RXInterrupt(void)
{ Serial.rxHandler();  _U1RXIF = 0; }
extern "C" void __attribute__((interrupt, no_auto_psv)) _U2RXInterrupt(void)
{ Serial1.rxHandler(); _U2RXIF = 0; }
extern "C" void __attribute__((interrupt, no_auto_psv)) _U3RXInterrupt(void)
{ Serial2.rxHandler(); _U3RXIF = 0; }

#elif defined(__dsPIC33A__)
// ---- dsPIC33AK UART (U1CON/U1STAT/U1BRG/U1TXB/U1RXB; 32-bit regs; poll-based) -
// AK's UART is a different peripheral than CK and its SFRs are 32-bit, so we
// ignore the (CK-shaped) uint16_t pointers and reach the module's registers
// directly, casting to the module-1 bitfield types for named fields.
//   FUART = FPB/2 = FCY/2 = F_CPU/4 (our F_CPU = 2*FCY). Legacy BRG (BRGS=0):
//   BRG = FUART/(16*baud) - 1 = F_CPU/(64*baud) - 1.
// PPS is unlocked out of reset (IOLOCK resets clear), so no key sequence needed.
// Only UART1 (Serial) is pin-wired by default, to the PKOB COM (TX RP59/RX RP58).

static volatile uint32_t *ak_con (uint8_t m) { return m == 1 ? &U1CON  : m == 2 ? &U2CON  : &U3CON;  }
static volatile uint32_t *ak_stat(uint8_t m) { return m == 1 ? &U1STAT : m == 2 ? &U2STAT : &U3STAT; }
static volatile uint32_t *ak_brg (uint8_t m) { return m == 1 ? &U1BRG  : m == 2 ? &U2BRG  : &U3BRG;  }
static volatile uint32_t *ak_txb (uint8_t m) { return m == 1 ? &U1TXB  : m == 2 ? &U2TXB  : &U3TXB;  }
static volatile uint32_t *ak_rxb (uint8_t m) { return m == 1 ? &U1RXB  : m == 2 ? &U2RXB  : &U3RXB;  }

#define AK_CON(m)  (*(volatile U1CONBITS  *)ak_con(m))
#define AK_STAT(m) (*(volatile U1STATBITS *)ak_stat(m))

// Map a remappable pin's OUTPUT function (RPORx, 7-bit fields, 4 per 32-bit reg).
// AK RPOR register numbers have gaps (RPOR3/7/11 absent), so use an explicit
// switch rather than &RPOR0+idx address arithmetic. No-op if the pin isn't
// output-remappable (idx lands on a missing register).
static void ak_rpor_set(uint8_t rp, uint8_t fn)
{
    if (rp < 1) return;
    uint8_t idx = (uint8_t)((rp - 1) / 4), sh = (uint8_t)(((rp - 1) % 4) * 8);
    volatile uint32_t *r;
    switch (idx) {
    case 0:  r=&RPOR0;  break; case 1:  r=&RPOR1;  break; case 2:  r=&RPOR2;  break;
    case 4:  r=&RPOR4;  break; case 5:  r=&RPOR5;  break; case 6:  r=&RPOR6;  break;
    case 8:  r=&RPOR8;  break; case 9:  r=&RPOR9;  break; case 10: r=&RPOR10; break;
    case 12: r=&RPOR12; break; case 13: r=&RPOR13; break; case 14: r=&RPOR14; break;
    case 15: r=&RPOR15; break; case 16: r=&RPOR16; break; case 17: r=&RPOR17; break;
    case 18: r=&RPOR18; break; case 19: r=&RPOR19; break;
    default: return;   // RPOR3/7/11 absent -> pin not output-remappable
    }
    *r = (*r & ~((uint32_t)0x7F << sh)) | ((uint32_t)(fn & 0x7F) << sh);
}

void HardwareSerial::begin(unsigned long baud)
{
    // Enable the module clock (PMD = Peripheral Module Disable; may default to
    // disabled, which gates the UART so U1CON writes have no effect).
    switch (_module) {
    case 1: _U1MD = 0; break;
    case 2: _U2MD = 0; break;
    case 3: _U3MD = 0; break;
    }
    __asm__ volatile ("nop"); __asm__ volatile ("nop");   // PMD change takes effect

    // Resolve native pin names -> RPn, make pins digital + directioned, map PPS.
    // (PPS is unlocked out of reset on AK — no key sequence needed.)
    uint8_t txRP = dspic_pin_to_rp(_txPin), rxRP = dspic_pin_to_rp(_rxPin);
    dspic_pin_pps_cfg(_txPin, 1);                // TX = output
    dspic_pin_pps_cfg(_rxPin, 0);                // RX = input
    uint8_t txfn = (_module == 1) ? 9 : (_module == 2) ? 11 : 13;  // U1/2/3 TX func
    if (txRP != 0xFF) ak_rpor_set(txRP, txfn);
    if (rxRP != 0xFF) switch (_module) {
    case 1: _U1RXR = rxRP; break;
    case 2: _U2RXR = rxRP; break;
    case 3: _U3RXR = rxRP; break;
    default: break;
    }

    volatile uint32_t *con = ak_con(_module);
    *con = 0;                     // MODE=0 (async 8N1), CLKSEL=0 (FPB/2), legacy BRG
    *ak_brg(_module) = (uint32_t)(F_CPU / (64UL * baud) - 1UL);
    AK_CON(_module).RXEN = 1;
    AK_CON(_module).TXEN = 1;
    AK_CON(_module).ON   = 1;
    _head = _tail = 0;
}

void HardwareSerial::end(void)        { AK_CON(_module).ON = 0; }
int  HardwareSerial::available(void)  { return AK_STAT(_module).RXBE ? 0 : 1; }
int  HardwareSerial::peek(void)       { return -1; }            // poll mode: no peek
int  HardwareSerial::read(void)
{
    if (AK_STAT(_module).RXBE) return -1;
    return (int)(uint8_t)*ak_rxb(_module);
}
size_t HardwareSerial::write(uint8_t c)
{
    // Bounded wait so a misconfigured/unclocked UART can't hang the sketch.
    for (uint32_t i = 0; i < 2000000UL && AK_STAT(_module).TXBF; i++) { }
    *ak_txb(_module) = c;
    return 1;
}
void HardwareSerial::rxHandler(void) { }                        // poll mode (no RX ISR)

HardwareSerial Serial (0, 0, 0, 0, 0, 0, 1);
HardwareSerial Serial1(0, 0, 0, 0, 0, 0, 2);
HardwareSerial Serial2(0, 0, 0, 0, 0, 0, 3);

#else  // ---- other families: compile-safe stub (UART differs per family) -------

void   HardwareSerial::begin(unsigned long baud) { (void)baud; }
void   HardwareSerial::begin(unsigned long, uint8_t, uint8_t) { }
void   HardwareSerial::end(void)                 { }
void   HardwareSerial::setPins(uint8_t, uint8_t) { }
int    HardwareSerial::available(void)           { return 0; }
int    HardwareSerial::peek(void)                { return -1; }
int    HardwareSerial::read(void)                { return -1; }
size_t HardwareSerial::write(uint8_t c)          { (void)c; return 1; }
void   HardwareSerial::rxHandler(void)           { }

HardwareSerial Serial (0, 0, 0, 0, 0, 0, 1);
HardwareSerial Serial1(0, 0, 0, 0, 0, 0, 2);
HardwareSerial Serial2(0, 0, 0, 0, 0, 0, 3);

#endif
