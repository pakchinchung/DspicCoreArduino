/*
 * EEPROM.cpp - flash-emulated EEPROM for dspicArduino.
 *
 * dsPIC33CK: backed by a reserved Flash page (1024 instruction words = the erase
 * unit), shadowed in RAM. Reads come from the RAM buffer; commit() erases the
 * page and reprograms it from the buffer using the NVM double-word sequence
 * (NVMOP=0001, NVMCON=0x4001) after a page erase (NVMOP=0011, NVMCON=0x4003).
 * See docs/PROJECT_STATUS.md §4/§6 for the datasheet-sourced register details.
 *
 * Storage layout: EEPROM byte i lives in Flash word (i/2), low byte if i is even,
 * high byte of the low 16 bits if odd. The instruction word's upper 8 bits
 * (23:16) are written 0. So 512 words hold EEPROM_SIZE=1024 bytes.
 *
 * NOTE: HW-UNVERIFIED as of 2026-06-24 (written from the datasheet; needs a real
 * dsPIC33CK to confirm the page alignment + NVM sequence — see the BootCounter
 * example). Other families get a RAM-only stub (no persistence).
 */
#include "EEPROM.h"

EEPROMClass EEPROM;

#define EE_WORDS      (EEPROM_SIZE / 2)   // Flash words used for data (512)
#define EE_PAGE_WORDS 1024                // dsPIC33CK page = 1024 instruction words

// RAM shadow of the EEPROM contents.
static uint8_t s_buf[EEPROM_SIZE];
static bool    s_loaded = false;

#if defined(__dsPIC33C__)
#include <xc.h>

// Reserve one full Flash page, page-aligned so a page erase hits exactly this
// array. Initialised to the erased state (0xFFFF) so a blank device reads 0xFF.
// (GNU range-init "[0 ... N]=v" is not valid C++, so fill via repetition macros.)
#define EE_F16(v)   v,v,v,v, v,v,v,v, v,v,v,v, v,v,v,v
#define EE_F256(v)  EE_F16(v),EE_F16(v),EE_F16(v),EE_F16(v), EE_F16(v),EE_F16(v),EE_F16(v),EE_F16(v), \
                    EE_F16(v),EE_F16(v),EE_F16(v),EE_F16(v), EE_F16(v),EE_F16(v),EE_F16(v),EE_F16(v)
#define EE_F1024(v) EE_F256(v),EE_F256(v),EE_F256(v),EE_F256(v)
static const __attribute__((space(prog), aligned(0x800)))
uint16_t s_flash[EE_PAGE_WORDS] = { EE_F1024(0xFFFF) };

void EEPROMClass::init()
{
    if (s_loaded) return;
    uint16_t page = __builtin_tblpage(s_flash);
    uint16_t off  = __builtin_tbloffset(s_flash);
    TBLPAG = page;
    for (uint16_t w = 0; w < EE_WORDS; w++) {
        uint16_t v = __builtin_tblrdl(off + (uint16_t)(w * 2));   // low 16 bits
        s_buf[w * 2]     = (uint8_t)(v & 0xFF);
        s_buf[w * 2 + 1] = (uint8_t)(v >> 8);
    }
    s_loaded = true;
}

bool EEPROMClass::commit()
{
    init();

    uint16_t page = __builtin_tblpage(s_flash);
    uint16_t off  = __builtin_tbloffset(s_flash);

    // --- 1) erase the page (NVMOP = 0011) ---
    NVMADRU = page;
    NVMADR  = off;
    NVMCON  = 0x4003;                 // WREN | page erase
    __builtin_write_NVM();            // unlock (0x55/0xAA) + set WR
    while (NVMCONbits.WR) { }
    if (NVMCONbits.WRERR) return false;

    // --- 2) reprogram in double-word units (NVMOP = 0001), 2 words at a time ---
    for (uint16_t w = 0; w < EE_WORDS; w += 2) {
        uint16_t w0 = (uint16_t)(s_buf[w * 2]       | (s_buf[w * 2 + 1] << 8));
        uint16_t w1 = (uint16_t)(s_buf[(w + 1) * 2] | (s_buf[(w + 1) * 2 + 1] << 8));

        TBLPAG = 0xFA;                // write-latch upper address (datasheet)
        __builtin_tblwtl(0, w0);  __builtin_tblwth(0, 0);   // first instr word
        __builtin_tblwtl(2, w1);  __builtin_tblwth(2, 0);   // second instr word

        NVMADRU = page;
        NVMADR  = (uint16_t)(off + w * 2);   // address of the first of the pair
        NVMCON  = 0x4001;                    // WREN | double-word program
        __builtin_write_NVM();
        while (NVMCONbits.WR) { }
        if (NVMCONbits.WRERR) return false;
    }
    return true;
}

#elif defined(__dsPIC33A__)
// dsPIC33AK: flash is 32-bit-word, programmed 128 bits (a "quad word" = 4x32-bit
// via NVMDATA0..3) at a time and erased per 4 KB page. No NVMKEY on this family —
// the PAC lets register writes through by default, so we just set NVMOP + WREN and
// pulse WR. A const array placed on its own page reserves the storage; its data
// pointer IS the NVM physical address (0x80xxxx), and const flash reads directly.
#include <xc.h>

#define EE_W32   (EEPROM_SIZE / 4)        // 32-bit flash words of data (256)
#define EE_PAGE  1024                     // 4 KB page = 1024 x 32-bit words

// Reserve a whole 4 KB page (page-aligned), pre-erased (0xFFFFFFFF -> reads 0xFF).
#define E4(v)    v,v,v,v
#define E16(v)   E4(v),E4(v),E4(v),E4(v)
#define E256(v)  E16(v),E16(v),E16(v),E16(v), E16(v),E16(v),E16(v),E16(v), \
                 E16(v),E16(v),E16(v),E16(v), E16(v),E16(v),E16(v),E16(v)
#define E1024(v) E256(v),E256(v),E256(v),E256(v)
static const __attribute__((aligned(0x1000)))
uint32_t s_flash[EE_PAGE] = { E1024(0xFFFFFFFFUL) };

void EEPROMClass::init()
{
    if (s_loaded) return;
    const volatile uint32_t *f = s_flash;          // volatile: always read flash
    for (uint16_t i = 0; i < EEPROM_SIZE; i++)
        s_buf[i] = (uint8_t)(f[i >> 2] >> ((i & 3) * 8));
    s_loaded = true;
}

static inline uint32_t ee_word(uint16_t w)         // pack 4 shadow bytes -> 32-bit
{
    uint16_t b = (uint16_t)(w * 4);
    return (uint32_t)s_buf[b] | ((uint32_t)s_buf[b+1] << 8)
         | ((uint32_t)s_buf[b+2] << 16) | ((uint32_t)s_buf[b+3] << 24);
}

bool EEPROMClass::commit()
{
    init();
    uint32_t base = (uint32_t)(uintptr_t)&s_flash[0];

    // --- 1) page erase (NVMOP = 0011) ---
    NVMADR = base;
    NVMCONbits.NVMOP = 0x3;
    NVMCONbits.WREN  = 1;
    __asm__ volatile ("nop"); __asm__ volatile ("nop");
    NVMCONbits.WR = 1;
    while (NVMCONbits.WR) { }
    if (NVMCONbits.WRERR) { NVMCONbits.WREN = 0; return false; }

    // --- 2) program 128 bits (NVMDATA0..3) per step, 16 bytes at a time ---
    for (uint16_t w = 0; w < EE_W32; w += 4) {
        NVMDATA0 = ee_word(w);
        NVMDATA1 = ee_word(w + 1);
        NVMDATA2 = ee_word(w + 2);
        NVMDATA3 = ee_word(w + 3);
        NVMADR = base + (uint32_t)w * 4;       // 16-byte-aligned quad-word address
        NVMCONbits.NVMOP = 0x1;
        NVMCONbits.WREN  = 1;
        __asm__ volatile ("nop"); __asm__ volatile ("nop");
        NVMCONbits.WR = 1;
        while (NVMCONbits.WR) { }
        if (NVMCONbits.WRERR) { NVMCONbits.WREN = 0; return false; }
    }
    NVMCONbits.WREN = 0;
    return true;
}

#else  // ---- non-CK: RAM-only stub (no persistence) --------------------------

void EEPROMClass::init()   { s_loaded = true; }
bool EEPROMClass::commit() { return false; }

#endif

// ---- EERef cell access (buffer-backed, family-independent) -----------------
uint8_t EERef::operator*() const
{
    EEPROM.init();
    if (index < 0 || index >= EEPROM_SIZE) return 0;
    return s_buf[index];
}

EERef &EERef::operator=(uint8_t v)
{
    EEPROM.init();
    if (index >= 0 && index < EEPROM_SIZE) s_buf[index] = v;
    return *this;
}
