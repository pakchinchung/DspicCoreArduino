#ifndef HardwareSerial_h
#define HardwareSerial_h

#include "Stream.h"
#include <stdint.h>

// TODO Phase 4: implement HardwareSerial backed by dsPIC UART1/UART2.
// Ring buffers + TX/RX ISRs live in HardwareSerial.cpp.

class HardwareSerial : public Stream
{
public:
    void begin(unsigned long baud);
    void end(void);

    virtual int  available(void);
    virtual int  peek(void);
    virtual int  read(void);
    virtual size_t write(uint8_t c);

    operator bool() { return true; }
};

extern HardwareSerial Serial;    // UART1
extern HardwareSerial Serial2;   // UART2

#endif
