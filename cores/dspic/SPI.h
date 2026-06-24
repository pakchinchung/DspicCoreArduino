#ifndef SPI_h
#define SPI_h
//
// Arduino SPI library for dsPIC33CK (SPI1, master mode).
//
// dsPIC pins are remappable (PPS). Defaults map to RD-port remappable pins;
// call SPI.setPins() BEFORE SPI.begin() to choose your own (dsPIC extension).
//   SCK  -> RP70 (RD6)   MOSI/SDO -> RP69 (RD5)   MISO/SDI -> RP71 (RD7)
//
// Standard Arduino API: begin/end, beginTransaction/endTransaction, transfer,
// transfer16, setBitOrder/setDataMode/setClockDivider.

#include "Arduino.h"

#define SPI_MODE0 0x00   // CPOL=0 CPHA=0
#define SPI_MODE1 0x01   // CPOL=0 CPHA=1
#define SPI_MODE2 0x02   // CPOL=1 CPHA=0
#define SPI_MODE3 0x03   // CPOL=1 CPHA=1

// Clock-divider constants (relative to FCY); mapped to a target frequency.
#define SPI_CLOCK_DIV2   2
#define SPI_CLOCK_DIV4   4
#define SPI_CLOCK_DIV8   8
#define SPI_CLOCK_DIV16  16
#define SPI_CLOCK_DIV32  32
#define SPI_CLOCK_DIV64  64
#define SPI_CLOCK_DIV128 128

class SPISettings {
public:
    SPISettings(uint32_t clockHz, uint8_t bitOrder, uint8_t dataMode)
        : clock(clockHz), order(bitOrder), mode(dataMode) {}
    SPISettings() : clock(1000000UL), order(MSBFIRST), mode(SPI_MODE0) {}
    uint32_t clock;
    uint8_t  order;
    uint8_t  mode;
};

class SPIClass {
public:
    // Bind this instance to one SPI module: pointers to its CON1L/CON1H/CON2L/
    // STATL/BUFL/BRGL registers + the module id (1..3, picks PPS codes).
    SPIClass(volatile uint16_t *con1l, volatile uint16_t *con1h,
             volatile uint16_t *con2l, volatile uint16_t *statl,
             volatile uint16_t *bufl,  volatile uint16_t *brgl,
             uint8_t moduleId);

    void begin();
    void end();

    // dsPIC extension: choose the remappable pins (RPn numbers) before begin().
    void setPins(uint8_t sckRP, uint8_t sdoRP, uint8_t sdiRP);

    void beginTransaction(SPISettings settings);
    void endTransaction();

    uint8_t  transfer(uint8_t data);
    uint16_t transfer16(uint16_t data);
    void     transfer(void *buf, size_t count);

    void setBitOrder(uint8_t order);
    void setDataMode(uint8_t mode);
    void setClockDivider(uint8_t div);

private:
    // module registers + id
    volatile uint16_t *_con1l, *_con1h, *_con2l, *_statl, *_bufl, *_brgl;
    uint8_t  _module;
    // RPn pin defaults (valid for SPI1/RD-port; set SPI1/SPI2 pins with setPins()).
    uint8_t  _sck = 70, _sdo = 69, _sdi = 71;
    uint8_t  _order = MSBFIRST;
    uint8_t  _mode  = SPI_MODE0;
    uint32_t _clock = 1000000UL;
    void applyConfig();
};

// Module instances. NOTE on naming: <xc.h> reserves the symbols SPI1/SPI2/SPI3
// (chip SFR accessors), so the extra buses are SPI_2 / SPI_3 (not SPI1/SPI2).
//   SPI   -> SPI1 peripheral (the standard Arduino object)
//   SPI_2 -> SPI2 peripheral
//   SPI_3 -> SPI3 peripheral
// SPI_2/SPI_3 use PPS — call setPins(sck, sdo, sdi) before begin().
extern SPIClass SPI;     // SPI1 peripheral
extern SPIClass SPI_2;   // SPI2 peripheral
extern SPIClass SPI_3;   // SPI3 peripheral

#endif
