#ifndef PGMSPACE_DSPIC_H
#define PGMSPACE_DSPIC_H
//
// Minimal AVR <avr/pgmspace.h> compatibility shim for dsPIC33.
//
// On AVR, PROGMEM data lives in a separate address space and needs the
// pgm_read_* accessors. On dsPIC33 with XC-DSC, `const` data is reachable with
// ordinary loads (the compiler manages flash/PSV access transparently — string
// literals and const tables already work in this core), so PROGMEM is a no-op
// and the pgm_read_* helpers are plain dereferences. This lets stock Arduino
// libraries that use PROGMEM / pgm_read_byte_near compile and run unchanged.
//
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef PGM_P
#define PGM_P const char *
#endif
#ifndef PGM_VOID_P
#define PGM_VOID_P const void *
#endif
#ifndef PSTR
#define PSTR(s) (s)
#endif
#ifndef F
#define F(s) (s)
#endif

typedef const char  prog_char;
typedef const unsigned char prog_uchar;
typedef const uint8_t  prog_uint8_t;
typedef const int8_t   prog_int8_t;
typedef const uint16_t prog_uint16_t;
typedef const int16_t  prog_int16_t;
typedef const uint32_t prog_uint32_t;
typedef const int32_t  prog_int32_t;

#define pgm_read_byte(addr)        (*(const uint8_t  *)(addr))
#define pgm_read_byte_near(addr)   (*(const uint8_t  *)(addr))
#define pgm_read_byte_far(addr)    (*(const uint8_t  *)(addr))
#define pgm_read_word(addr)        (*(const uint16_t *)(addr))
#define pgm_read_word_near(addr)   (*(const uint16_t *)(addr))
#define pgm_read_word_far(addr)    (*(const uint16_t *)(addr))
#define pgm_read_dword(addr)       (*(const uint32_t *)(addr))
#define pgm_read_dword_near(addr)  (*(const uint32_t *)(addr))
#define pgm_read_float(addr)       (*(const float    *)(addr))
#define pgm_read_ptr(addr)         (*(const void * const *)(addr))

// Flash string/mem functions map to libc (data is directly addressable).
#define strcpy_P    strcpy
#define strncpy_P   strncpy
#define strcat_P    strcat
#define strncat_P   strncat
#define strcmp_P    strcmp
#define strncmp_P   strncmp
#define strcasecmp_P strcasecmp
#define strlen_P    strlen
#define strstr_P    strstr
#define memcpy_P    memcpy
#define memcmp_P    memcmp
#define sprintf_P   sprintf
#define snprintf_P  snprintf

#endif /* PGMSPACE_DSPIC_H */
