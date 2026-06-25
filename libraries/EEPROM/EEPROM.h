/*
 * EEPROM.h - flash-emulated EEPROM for dspicArduino (dsPIC33CK).
 *
 * The dsPIC33CK has no real EEPROM, so this emulates one in a reserved Flash
 * page (1024 instruction words = the erase unit). The page is shadowed in a RAM
 * buffer: read()/[] return the buffer; write()/update()/put() modify it; commit()
 * erases the page and writes the buffer back via the NVM double-word sequence.
 *
 * IMPORTANT (differs from AVR): writes are buffered — call EEPROM.commit() to
 * persist them to Flash (same convention as the ESP/SAMD cores). This avoids a
 * full page erase+rewrite on every byte (slow, and wears Flash). Data survive a
 * reset after commit(); a *reprogram* (ICSP) bulk-erases the page.
 *
 * API matches the stock Arduino EEPROM library: read, write, update, get, put,
 * length, operator[], plus begin()/commit()/end() for the flash backing.
 */
#ifndef dspicArduino_EEPROM_h
#define dspicArduino_EEPROM_h

#include <Arduino.h>

// Emulated EEPROM size in bytes. Backed by a Flash page that holds 1024 16-bit
// words (2 bytes each) = 2048 B capacity; we expose 1024 B and keep headroom.
#ifndef EEPROM_SIZE
#define EEPROM_SIZE 1024
#endif

// ---- EERef: a reference to one EEPROM cell (enables EEPROM[i] = v; etc.) ----
struct EERef {
    EERef(int idx) : index(idx) {}

    uint8_t operator*() const;                 // read
    operator uint8_t() const { return **this; }
    EERef &operator=(uint8_t v);               // write (to RAM buffer)
    EERef &operator=(const EERef &ref) { return *this = (uint8_t)ref; }

    EERef &update(uint8_t v) { return (v != **this) ? (*this = v) : *this; }

    // Arithmetic-assignment helpers (parity with the stock library).
    EERef &operator+=(uint8_t v) { return *this = (uint8_t)(**this + v); }
    EERef &operator-=(uint8_t v) { return *this = (uint8_t)(**this - v); }
    EERef &operator*=(uint8_t v) { return *this = (uint8_t)(**this * v); }
    EERef &operator/=(uint8_t v) { return *this = (uint8_t)(**this / v); }
    EERef &operator^=(uint8_t v) { return *this = (uint8_t)(**this ^ v); }
    EERef &operator%=(uint8_t v) { return *this = (uint8_t)(**this % v); }
    EERef &operator&=(uint8_t v) { return *this = (uint8_t)(**this & v); }
    EERef &operator|=(uint8_t v) { return *this = (uint8_t)(**this | v); }
    EERef &operator<<=(uint8_t v){ return *this = (uint8_t)(**this << v); }
    EERef &operator>>=(uint8_t v){ return *this = (uint8_t)(**this >> v); }

    EERef &operator++() { return *this += 1; }
    EERef &operator--() { return *this -= 1; }
    uint8_t operator++(int) { uint8_t r = **this; ++(*this); return r; }
    uint8_t operator--(int) { uint8_t r = **this; --(*this); return r; }

    int index;
};

// ---- EEPtr: a pointer over EEPROM cells (enables iteration) -----------------
struct EEPtr {
    EEPtr(int idx) : index(idx) {}
    operator int() const { return index; }
    EEPtr &operator=(int in) { index = in; return *this; }

    EERef operator*() { return index; }
    bool operator!=(const EEPtr &p) const { return index != p.index; }
    EEPtr &operator++() { ++index; return *this; }
    EEPtr &operator--() { --index; return *this; }
    EEPtr operator++(int) { return index++; }
    EEPtr operator--(int) { return index--; }

    int index;
};

class EEPROMClass {
public:
    // Cell access.
    EERef operator[](int idx)       { return idx; }
    uint8_t read(int idx)           { return EERef(idx); }
    void    write(int idx, uint8_t v)  { (EERef(idx)) = v; }   // parens: not a decl
    void    update(int idx, uint8_t v) { EERef(idx).update(v); }

    uint16_t length() const { return EEPROM_SIZE; }

    // Iteration.
    EEPtr begin() { return 0; }                 // NB: this is iterator-begin, not
    EEPtr end()   { return length(); }          // the flash begin() below.

    // Bulk typed access (memcpy to/from the RAM buffer).
    template <typename T> T &get(int idx, T &t);
    template <typename T> const T &put(int idx, const T &t);

    // ---- flash backing ----
    void init();        // load the Flash page into the RAM buffer (idempotent)
    bool commit();      // erase the page + write the RAM buffer back to Flash
    void begin(size_t) { init(); }   // ESP-style begin(size) (size fixed here);
                                     // note end() is iterator-end above — persist
                                     // with commit(), not end().
};

extern EEPROMClass EEPROM;

// ---- templates (must be in the header) --------------------------------------
template <typename T> T &EEPROMClass::get(int idx, T &t)
{
    uint8_t *p = (uint8_t *)&t;
    for (size_t i = 0; i < sizeof(T); i++) p[i] = read(idx + (int)i);
    return t;
}

template <typename T> const T &EEPROMClass::put(int idx, const T &t)
{
    const uint8_t *p = (const uint8_t *)&t;
    for (size_t i = 0; i < sizeof(T); i++) write(idx + (int)i, p[i]);
    return t;
}

#endif /* dspicArduino_EEPROM_h */
