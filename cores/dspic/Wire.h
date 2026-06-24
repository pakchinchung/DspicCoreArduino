#ifndef Wire_h
#define Wire_h
//
// Arduino Wire (I2C) library for dsPIC33CK — master mode, multi-instance.
//
//   Wire   -> I2C1      Wire1 -> I2C2      Wire2 -> I2C3
//
// I2C SCL/SDA are NOT PPS-remappable on this device (unlike SPI/UART). Each
// module has a fixed primary pin pair and an alternate pair, selected by the
// ALTI2Cx FDEVOPT config bit (compile-time, see the variant). On this board
// I2C1 defaults to its ALTERNATE pins RC8 (SDA) / RC9 (SCL); build with
// -DDSPIC_I2C1_PRIMARY to use RB8/RB9 instead (or the Tools > "I2C1 pins" menu).
//
// Standard API: begin, setClock, beginTransmission, write, endTransmission,
// requestFrom, available, read, peek. begin(scl,sda) is a dsPIC convenience.
// (Slave mode is a planned follow-up.)

#include "Arduino.h"

#define WIRE_BUFFER_SIZE 32

class TwoWire {
public:
    // Bind this instance to one I2C module: pointers to its CONL/BRG/TRN/RCV/
    // STAT registers + the module id (1..3). Instances are created in Wire.cpp.
    TwoWire(volatile uint16_t *conl, volatile uint16_t *brg,
            volatile uint16_t *trn,  volatile uint16_t *rcv,
            volatile uint16_t *stat, uint8_t moduleId);

    void begin();                                    // master, compiled pins
    void begin(uint8_t sclPin, uint8_t sdaPin);      // dsPIC: pins for readability
    void setClock(uint32_t hz);

    void    beginTransmission(uint8_t address);
    size_t  write(uint8_t data);
    size_t  write(const uint8_t *data, size_t n);
    uint8_t endTransmission(bool sendStop = true);   // 0 = success

    uint8_t requestFrom(uint8_t address, uint8_t quantity, bool sendStop = true);
    int     available();
    int     read();
    int     peek();

private:
    // module registers + id
    volatile uint16_t *_conl, *_brg, *_trn, *_rcv, *_stat;
    uint8_t  _module;

    // low-level master primitives (operate on this instance's module)
    void    setbrg(uint32_t fscl);
    void    pinSetup();
    void    start();
    void    rstart();
    void    stop();
    bool    tx(uint8_t b);          // true if ACKed
    uint8_t rx(bool ack);

    uint8_t  _txAddr = 0;
    uint8_t  _txBuf[WIRE_BUFFER_SIZE];
    uint8_t  _txLen = 0;
    uint8_t  _rxBuf[WIRE_BUFFER_SIZE];
    uint8_t  _rxLen = 0;
    uint8_t  _rxIdx = 0;
    uint32_t _clock = 100000UL;
};

extern TwoWire Wire;     // I2C1
extern TwoWire Wire1;    // I2C2
extern TwoWire Wire2;    // I2C3

#endif
