#ifndef HardwareSerial_h
#define HardwareSerial_h

#include "Stream.h"
#include <stdint.h>

// HardwareSerial for dsPIC33CK "new" UART, multi-instance:
//   Serial -> UART1 (PKoB4 virtual COM, RD3/RD4)   Serial1 -> UART2   Serial2 -> UART3
// UART TX/RX are PPS-remappable; Serial defaults to the on-board COM pins, while
// Serial1/Serial2 need setPins(txRP, rxRP) before begin().

#define SERIAL_RX_BUFSIZE 64
#define SERIAL_RX_MASK    (SERIAL_RX_BUFSIZE - 1)

class HardwareSerial : public Stream
{
public:
    // Bind to one UART module: pointers to its MODE/MODEH/STAH/BRG/RXREG/TXREG
    // registers + module id (1..3). Instances are created in HardwareSerial.cpp.
    HardwareSerial(volatile uint16_t *mode,  volatile uint16_t *modeH,
                   volatile uint16_t *staH,  volatile uint16_t *brg,
                   volatile uint16_t *rxreg, volatile uint16_t *txreg,
                   uint8_t moduleId);

    void begin(unsigned long baud);
    void end(void);
    void setPins(uint8_t txRP, uint8_t rxRP);   // dsPIC: PPS pins before begin()

    virtual int  available(void);
    virtual int  peek(void);
    virtual int  read(void);
    virtual size_t write(uint8_t c);

    operator bool() { return true; }

    void rxHandler(void);                        // called from the per-module RX ISR

private:
    volatile uint16_t *_mode, *_modeH, *_staH, *_brg, *_rxreg, *_txreg;
    uint8_t  _module;
    uint8_t  _txRP = 68, _rxRP = 67;             // PPS defaults (Serial = U1, RD4/RD3)
    volatile uint8_t _rx[SERIAL_RX_BUFSIZE];
    volatile uint8_t _head = 0, _tail = 0;
};

extern HardwareSerial Serial;    // UART1
extern HardwareSerial Serial1;   // UART2
extern HardwareSerial Serial2;   // UART3

#endif
