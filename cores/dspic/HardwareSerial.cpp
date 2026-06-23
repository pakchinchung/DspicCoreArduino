// HardwareSerial — UART1 for dsPIC33CK (the "new" UART module).
//
// dsPIC33CK only. On dsPIC33A (and any other family) we keep a compile-safe
// stub (its UART/PPS differ and aren't verified yet).
//
// Default pin mapping (DM330030 on-board PKoB4 virtual COM):
//   U1TX -> RP68 / RD4   (_RP68R = U1TX)
//   U1RX <- RP67 / RD3   (_U1RXR = 67)

#include "HardwareSerial.h"
#include "Arduino.h"
#include <xc.h>

#if defined(__dsPIC33C__)

// The UART baud generator (BCLKSEL=0) runs from FP = FCY = F_CPU/2, matching
// the Timer1 timing in wiring.cpp. (Empirically confirmed: dividing by F_CPU
// gave exactly half the requested baud.)
#ifndef F_CPU
#define F_CPU 100000000UL
#endif
#define UART_FCY  (F_CPU / 2UL)

#define SERIAL_RX_BUFSIZE 64
#define SERIAL_RX_MASK    (SERIAL_RX_BUFSIZE - 1)

static volatile uint8_t s_rx[SERIAL_RX_BUFSIZE];
static volatile uint8_t s_head = 0, s_tail = 0;

void HardwareSerial::begin(unsigned long baud)
{
    // --- map U1TX/U1RX onto the PKoB4 pins via PPS --------------------------
    TRISDbits.TRISD3 = 1;                    // RD3 (RX) input
    __builtin_write_RPCON(0x0000);           // unlock PPS (IOLOCK = 0)
    _U1RXR = 67;                             // U1RX  <- RP67 (RD3)
    _RP68R = _RPOUT_U1TX;                     // RP68 (RD4) -> U1TX
    __builtin_write_RPCON(0x0800);           // lock PPS (IOLOCK = 1)

    // --- UART1: 8 data bits, no parity, 1 stop; BRGH=0 (÷16); clock = FP ----
    U1MODE  = 0;
    U1MODEbits.MOD     = 0;                  // 8-bit data, no parity
    U1MODEbits.BRGH    = 0;                  // 16x oversampling
    U1MODEHbits.BCLKSEL = 0;                 // baud clock = FP (= FCY)
    U1BRG  = (uint16_t)((UART_FCY / (16UL * baud)) - 1UL);

    U1MODEbits.UARTEN  = 1;                  // enable module
    U1MODEbits.UTXEN   = 1;                  // enable transmitter
    U1MODEbits.URXEN   = 1;                  // enable receiver

    // --- RX interrupt (U1RX is in IFS0/IEC0/IPC2 on dsPIC33CK) --------------
    s_head = s_tail = 0;
    _U1RXIF = 0;
    _U1RXIP = 4;
    _U1RXIE = 1;
}

void HardwareSerial::end(void)
{
    _U1RXIE = 0;
    U1MODEbits.UARTEN = 0;
}

int HardwareSerial::available(void)
{
    return (uint8_t)(s_head - s_tail) & SERIAL_RX_MASK;
}

int HardwareSerial::peek(void)
{
    if (s_head == s_tail) return -1;
    return s_rx[s_tail];
}

int HardwareSerial::read(void)
{
    if (s_head == s_tail) return -1;
    uint8_t c = s_rx[s_tail];
    s_tail = (s_tail + 1) & SERIAL_RX_MASK;
    return c;
}

size_t HardwareSerial::write(uint8_t c)
{
    while (U1STAHbits.UTXBF) { }             // wait while TX FIFO full
    U1TXREG = c;
    return 1;
}

extern "C" void __attribute__((interrupt, no_auto_psv)) _U1RXInterrupt(void)
{
    while (!U1STAHbits.URXBE) {               // while RX FIFO not empty
        uint8_t c = (uint8_t)U1RXREG;
        uint8_t next = (s_head + 1) & SERIAL_RX_MASK;
        if (next != s_tail) {                 // drop on overflow
            s_rx[s_head] = c;
            s_head = next;
        }
    }
    _U1RXIF = 0;
}

HardwareSerial Serial;    // UART1 on the PKoB4 virtual COM (RD3/RD4)
HardwareSerial Serial2;   // (UART2 not yet wired; shares the stub behavior)

#else  // ---- non-dsPIC33C: compile-safe stub (UART differs per family) -------

void   HardwareSerial::begin(unsigned long baud) { (void)baud; }
void   HardwareSerial::end(void)                 { }
int    HardwareSerial::available(void)           { return 0; }
int    HardwareSerial::peek(void)                { return -1; }
int    HardwareSerial::read(void)                { return -1; }
size_t HardwareSerial::write(uint8_t c)          { (void)c; return 1; }

HardwareSerial Serial;
HardwareSerial Serial2;

#endif
