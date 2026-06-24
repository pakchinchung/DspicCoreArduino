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

void HardwareSerial::setPins(uint8_t txRP, uint8_t rxRP)
{
    _txRP = txRP; _rxRP = rxRP;
}

void HardwareSerial::begin(unsigned long baud)
{
    // --- PPS: route TX (output func) and RX (input select) per module --------
    __builtin_write_RPCON(0x0000);               // unlock PPS
    switch (_module) {
    case 1: _U1RXR = _rxRP; rpor_set(_txRP, 1);  break;   // U1TX = 1
    case 2: _U2RXR = _rxRP; rpor_set(_txRP, 3);  break;   // U2TX = 3
    case 3: _U3RXR = _rxRP; rpor_set(_txRP, 27); break;   // U3TX = 27
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

#else  // ---- non-dsPIC33C: compile-safe stub (UART differs per family) -------

void   HardwareSerial::begin(unsigned long baud) { (void)baud; }
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
