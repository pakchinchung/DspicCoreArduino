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

void SPIClass::setPins(uint8_t sckPin, uint8_t sdoPin, uint8_t sdiPin)
{
    _sck = sckPin; _sdo = sdoPin; _sdi = sdiPin;   // native pin names; applied in begin()
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

    // Resolve native pin names -> RPn and prep the pads (digital + direction).
    uint8_t sckRP = dspic_pin_to_rp(_sck);
    uint8_t sdoRP = dspic_pin_to_rp(_sdo);
    uint8_t sdiRP = dspic_pin_to_rp(_sdi);
    dspic_pin_pps_cfg(_sck, 1);                // SCK = output
    dspic_pin_pps_cfg(_sdo, 1);                // SDO = output
    dspic_pin_pps_cfg(_sdi, 0);                // SDI = input

    __builtin_write_RPCON(0x0000);             // unlock PPS
    switch (_module) {                         // PPS in/out per module
    case 1: if (sdiRP != 0xFF) _SDI1R = sdiRP; if (sdoRP != 0xFF) rpor_set(sdoRP, 5);  if (sckRP != 0xFF) rpor_set(sckRP, 6);  break;
    case 2: if (sdiRP != 0xFF) _SDI2R = sdiRP; if (sdoRP != 0xFF) rpor_set(sdoRP, 8);  if (sckRP != 0xFF) rpor_set(sckRP, 9);  break;
    case 3: if (sdiRP != 0xFF) _SDI3R = sdiRP; if (sdoRP != 0xFF) rpor_set(sdoRP, 11); if (sckRP != 0xFF) rpor_set(sckRP, 12); break;
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

#elif defined(__dsPIC33A__)
// ---- dsPIC33AK SPI (SPI1, master) ------------------------------------------
// AK SPI uses a single 32-bit SPI1CON1 (not CK's CON1L/CON1H split). Only SPI1
// (the `SPI` object) is wired here; SPI_2/SPI_3 would need SPI2*/SPI3* + a
// switch(_module). SDO/SCK are PPS OUTPUTS (codes SDO1=13, SCK1OUT=14); SDI is a
// PPS INPUT (_SDI1R). PPS is unlocked out of reset. Pins are native names.

#define SPI_FCY (F_CPU / 2UL)

// AK RPOR output setter (RPOR register numbers have gaps -> explicit switch).
static void ak_rpor_set(uint8_t rp, uint8_t fn)
{
    if (rp < 1) return;
    uint8_t idx = (uint8_t)((rp - 1) / 4), sh = (uint8_t)(((rp - 1) % 4) * 8);
    volatile uint32_t *r;
    switch (idx) {
    case 0:r=&RPOR0;break;  case 1:r=&RPOR1;break;  case 2:r=&RPOR2;break;
    case 4:r=&RPOR4;break;  case 5:r=&RPOR5;break;  case 6:r=&RPOR6;break;
    case 8:r=&RPOR8;break;  case 9:r=&RPOR9;break;  case 10:r=&RPOR10;break;
    case 12:r=&RPOR12;break;case 13:r=&RPOR13;break;case 14:r=&RPOR14;break;
    case 15:r=&RPOR15;break;case 16:r=&RPOR16;break;case 17:r=&RPOR17;break;
    case 18:r=&RPOR18;break;case 19:r=&RPOR19;break;
    default: return;
    }
    *r = (*r & ~((uint32_t)0x7F << sh)) | ((uint32_t)(fn & 0x7F) << sh);
}

static inline uint8_t reverse8(uint8_t b)
{
    b = (uint8_t)((b >> 4) | (b << 4));
    b = (uint8_t)(((b & 0xCC) >> 2) | ((b & 0x33) << 2));
    b = (uint8_t)(((b & 0xAA) >> 1) | ((b & 0x55) << 1));
    return b;
}

// Per-module register access (SPI1/2/3 share SPI1's bitfield layout).
static volatile uint32_t *ak_scon1(uint8_t m){ return m==1?&SPI1CON1: m==2?&SPI2CON1: &SPI3CON1; }
static volatile uint32_t *ak_sstat(uint8_t m){ return m==1?&SPI1STAT: m==2?&SPI2STAT: &SPI3STAT; }
static volatile uint32_t *ak_sbrg (uint8_t m){ return m==1?&SPI1BRG : m==2?&SPI2BRG : &SPI3BRG ; }
static volatile uint32_t *ak_sbuf (uint8_t m){ return m==1?&SPI1BUF : m==2?&SPI2BUF : &SPI3BUF ; }
#define SCON1(m) (*(volatile SPI1CON1BITS *)ak_scon1(m))
#define SSTAT(m) (*(volatile SPI1STATBITS *)ak_sstat(m))

void SPIClass::setPins(uint8_t sckPin, uint8_t sdoPin, uint8_t sdiPin)
{
    _sck = sckPin; _sdo = sdoPin; _sdi = sdiPin;   // native pin names; applied in begin()
}

void SPIClass::applyConfig()
{
    uint8_t ckp = (_mode & 0x02) ? 1 : 0;          // CPOL
    uint8_t cke = (_mode & 0x01) ? 0 : 1;          // dsPIC CKE = !CPHA
    SCON1(_module).ON  = 0;
    SCON1(_module).CKP = ckp;
    SCON1(_module).CKE = cke;
    uint32_t brg = SPI_FCY / (2UL * (_clock ? _clock : 1UL));
    if (brg) brg -= 1;
    if (brg > 8191UL) brg = 8191UL;
    *ak_sbrg(_module) = (uint32_t)brg;
    SCON1(_module).ON  = 1;
}

void SPIClass::begin()
{
    switch (_module) {                             // enable module clock (PMD)
    case 1: _SPI1MD = 0; break;
    case 2: _SPI2MD = 0; break;
    case 3: _SPI3MD = 0; break;
    }
    __asm__ volatile ("nop"); __asm__ volatile ("nop");
    *ak_scon1(_module) = 0; *ak_sstat(_module) = 0;

    uint8_t sckRP = dspic_pin_to_rp(_sck);
    uint8_t sdoRP = dspic_pin_to_rp(_sdo);
    uint8_t sdiRP = dspic_pin_to_rp(_sdi);
    dspic_pin_pps_cfg(_sck, 1);                    // SCK = output
    dspic_pin_pps_cfg(_sdi, 0);                    // SDI = input ...
    dspic_pin_pps_cfg(_sdo, 1);                    // ... SDO = output (last, so an
                                                   // SDI==SDO loopback pin ends output)
    // PPS output func codes: SDO/SCK = 13/14 (SPI1), 16/17 (SPI2), 19/20 (SPI3).
    uint8_t sdofn = (_module==1)?13 : (_module==2)?16 : 19;
    uint8_t sckfn = (_module==1)?14 : (_module==2)?17 : 20;
    if (sckRP != 0xFF) ak_rpor_set(sckRP, sckfn);
    if (sdoRP != 0xFF) ak_rpor_set(sdoRP, sdofn);
    if (sdiRP != 0xFF) switch (_module) {          // SDI input select
    case 1: _SDI1R = sdiRP; break;
    case 2: _SDI2R = sdiRP; break;
    case 3: _SDI3R = sdiRP; break;
    }

    SCON1(_module).MSTEN  = 1;                      // master
    SCON1(_module).MODE16 = 0;                      // 8-bit
    SCON1(_module).MODE32 = 0;
    SCON1(_module).ENHBUF = 0;                      // standard (non-FIFO) buffer
    applyConfig();
}

void SPIClass::end() { SCON1(_module).ON = 0; }

void SPIClass::beginTransaction(SPISettings s)
{
    _clock = s.clock; _order = s.order; _mode = s.mode;
    applyConfig();
}
void SPIClass::endTransaction() { }

uint8_t SPIClass::transfer(uint8_t data)
{
    if (_order == LSBFIRST) data = reverse8(data);
    *ak_sbuf(_module) = data;
    for (uint32_t g = 0; g < 200000UL && !SSTAT(_module).SPIRBF; g++) { }
    uint8_t r = (uint8_t)*ak_sbuf(_module);
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

SPIClass SPI  (0, 0, 0, 0, 0, 0, 1);
SPIClass SPI_2(0, 0, 0, 0, 0, 0, 2);
SPIClass SPI_3(0, 0, 0, 0, 0, 0, 3);

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
