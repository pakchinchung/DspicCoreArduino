#ifndef HardwareSerial_h
#define HardwareSerial_h

#include "Stream.h"
#include <stdint.h>
#include "dspic_pins.h"   // native pin-name ids (RA0..RG15) for setPins()/begin()

// HardwareSerial for the dsPIC "new"/AK UART, multi-instance:
//   Serial -> UART1   Serial1 -> UART2   Serial2 -> UART3
// UART TX/RX are PPS-remappable, so the pins are chosen at runtime by native pin
// NAME (e.g. RD4) — either Serial.setPins(txPin, rxPin) before begin(), or the
// begin(baud, txPin, rxPin) overload (ESP32-style). Serial defaults to the
// on-board COM pins; Serial1/Serial2 must be given pins before use.

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
    void begin(unsigned long baud, uint8_t txPin, uint8_t rxPin);  // ESP32-style
    void end(void);
    void setPins(uint8_t txPin, uint8_t rxPin);  // native pin NAMEs, before begin()

    virtual int  available(void);
    virtual int  peek(void);
    virtual int  read(void);
    virtual size_t write(uint8_t c);

    operator bool() { return true; }

    void rxHandler(void);                        // called from the per-module RX ISR

private:
    volatile uint16_t *_mode, *_modeH, *_staH, *_brg, *_rxreg, *_txreg;
    uint8_t  _module;
    // Serial (UART1) default pins, by native name; setPins()/begin() override.
#if defined(__dsPIC33A__)
    uint8_t  _txPin = RD1, _rxPin = RD3;         // AK Serial: MCP2221A COM (TX=RD1,RX=RD3)
#else
    uint8_t  _txPin = RD4, _rxPin = RD3;         // CK Serial: PKoB COM (TX=RD4,RX=RD3)
#endif
    volatile uint8_t _rx[SERIAL_RX_BUFSIZE];
    volatile uint8_t _head = 0, _tail = 0;
};

extern HardwareSerial Serial;    // UART1
extern HardwareSerial Serial1;   // UART2
extern HardwareSerial Serial2;   // UART3

#endif
