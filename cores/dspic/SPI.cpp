// Arduino SPI for dsPIC33CK, multi-instance (SPI1/2/3, master).
// Non-CK builds get a stub.
#include "SPI.h"
#include "xc_compat.h"   // <xc.h> with its generic 'SPI' typedef renamed away

SPIClass::SPIClass(volatile uint16_t *con1l, volatile uint16_t *con1h,
                   volatile uint16_t *con2l, volatile uint16_t *statl,
                   volatile uint16_t *bufl,  volatile uint16_t *brgl,
                   uint8_t moduleId)
    : _con1l(con1l), _con1h(con1h), _con2l(con2l),
      _statl(statl), _bufl(bufl), _brgl(brgl), _module(moduleId)
{
}

#if defined(__dsPIC33C__)

#define SPI_FCY (F_CPU / 2UL)        // peripheral/instruction clock (50 MHz here)

// All SPIx modules share module 1's bit layout, so reach the control/status
// bits through the stored register pointer cast to module-1's bitfield types.
#define CON1LB (*(volatile SPI1CON1LBITS *)_con1l)
#define STATLB (*(volatile SPI1STATLBITS *)_statl)

// Map a remappable pin's output function. RPORx are contiguous 16-bit words,
// two 6-bit fields each; RPn 32 is RPOR0 low byte. (module-independent)
static void rpor_set(uint8_t rp, uint8_t fn)
{
    volatile uint16_t *rpor = (&RPOR0) + ((rp - 32) >> 1);
    uint8_t sh = ((rp - 32) & 1) ? 8 : 0;
    *rpor = (uint16_t)((*rpor & ~(0x3Fu << sh)) | ((uint16_t)(fn & 0x3F) << sh));
}

static inline uint8_t reverse8(uint8_t b)
{
    b = (uint8_t)((b >> 4) | (b << 4));
    b = (uint8_t)(((b & 0xCC) >> 2) | ((b & 0x33) << 2));
    b = (uint8_t)(((b & 0xAA) >> 1) | ((b & 0x55) << 1));
    return b;
}

void SPIClass::setPins(uint8_t sckRP, uint8_t sdoRP, uint8_t sdiRP)
{
    _sck = sckRP; _sdo = sdoRP; _sdi = sdiRP;
}

void SPIClass::applyConfig()
{
    uint8_t ckp = (_mode & 0x02) ? 1 : 0;      // CPOL
    uint8_t cke = (_mode & 0x01) ? 0 : 1;      // dsPIC CKE = !CPHA

    CON1LB.SPIEN = 0;
    CON1LB.CKP = ckp;
    CON1LB.CKE = cke;

    // Fsck = FCY / (2 * (BRG + 1))  ->  BRG = FCY/(2*Fsck) - 1
    uint32_t brg = SPI_FCY / (2UL * (_clock ? _clock : 1UL));
    if (brg) brg -= 1;
    if (brg > 8191UL) brg = 8191UL;
    *_brgl = (uint16_t)brg;

    CON1LB.SPIEN = 1;
}

void SPIClass::begin()
{
    *_con1l = 0; *_con1h = 0; *_con2l = 0; *_statl = 0;

    __builtin_write_RPCON(0x0000);             // unlock PPS
    switch (_module) {                         // PPS in/out per module
    case 1: _SDI1R = _sdi; rpor_set(_sdo, 5);  rpor_set(_sck, 6);  break;
    case 2: _SDI2R = _sdi; rpor_set(_sdo, 8);  rpor_set(_sck, 9);  break;
    case 3: _SDI3R = _sdi; rpor_set(_sdo, 11); rpor_set(_sck, 12); break;
    default: break;
    }
    __builtin_write_RPCON(0x0800);             // lock PPS

    CON1LB.MSTEN  = 1;                          // master
    CON1LB.MODE16 = 0;                          // 8-bit
    CON1LB.MODE32 = 0;
    CON1LB.ENHBUF = 0;                          // standard (non-FIFO) buffer
    applyConfig();
}

void SPIClass::end()
{
    CON1LB.SPIEN = 0;
}

void SPIClass::beginTransaction(SPISettings s)
{
    _clock = s.clock; _order = s.order; _mode = s.mode;
    applyConfig();
}

void SPIClass::endTransaction() { }

uint8_t SPIClass::transfer(uint8_t data)
{
    if (_order == LSBFIRST) data = reverse8(data);
    *_bufl = data;
    while (!STATLB.SPIRBF) { }                  // wait for the exchange to finish
    uint8_t r = (uint8_t)(*_bufl);
    if (_order == LSBFIRST) r = reverse8(r);
    return r;
}

uint16_t SPIClass::transfer16(uint16_t data)
{
    uint8_t hi = transfer((uint8_t)(data >> 8));
    uint8_t lo = transfer((uint8_t)(data & 0xFF));
    return (uint16_t)((hi << 8) | lo);
}

void SPIClass::transfer(void *buf, size_t count)
{
    uint8_t *p = (uint8_t *)buf;
    while (count--) { *p = transfer(*p); p++; }
}

void SPIClass::setBitOrder(uint8_t order) { _order = order; }
void SPIClass::setDataMode(uint8_t mode)  { _mode = mode; applyConfig(); }
void SPIClass::setClockDivider(uint8_t div)
{
    _clock = SPI_FCY / (div ? div : 4);
    applyConfig();
}

// ---- the three module instances (SPI1/2/3 are reserved xc.h SFR names) ----
SPIClass SPI  (&SPI1CON1L, &SPI1CON1H, &SPI1CON2L, &SPI1STATL, &SPI1BUFL, &SPI1BRGL, 1);
SPIClass SPI_2(&SPI2CON1L, &SPI2CON1H, &SPI2CON2L, &SPI2STATL, &SPI2BUFL, &SPI2BRGL, 2);
SPIClass SPI_3(&SPI3CON1L, &SPI3CON1H, &SPI3CON2L, &SPI3STATL, &SPI3BUFL, &SPI3BRGL, 3);

#else  // ---- non-dsPIC33C stub -------------------------------------------------

void     SPIClass::begin() {}
void     SPIClass::end() {}
void     SPIClass::setPins(uint8_t, uint8_t, uint8_t) {}
void     SPIClass::beginTransaction(SPISettings) {}
void     SPIClass::endTransaction() {}
uint8_t  SPIClass::transfer(uint8_t d) { return d; }
uint16_t SPIClass::transfer16(uint16_t d) { return d; }
void     SPIClass::transfer(void *, size_t) {}
void     SPIClass::setBitOrder(uint8_t) {}
void     SPIClass::setDataMode(uint8_t) {}
void     SPIClass::setClockDivider(uint8_t) {}
void     SPIClass::applyConfig() {}

SPIClass SPI  (0, 0, 0, 0, 0, 0, 1);
SPIClass SPI_2(0, 0, 0, 0, 0, 0, 2);
SPIClass SPI_3(0, 0, 0, 0, 0, 0, 3);

#endif
