#ifndef XC_COMPAT_H
#define XC_COMPAT_H
//
// <xc.h> defines unguarded "generic peripheral SFR-block" typedefs named SPI
// and UART (plus PSPI/PUART). These collide with Arduino's `SPI` object (and a
// future `UART`). We never use those generic structs — register access goes
// through the explicit SFRs (SPI1CON1L, U1MODE, ...) — so rename them out of
// the way around the <xc.h> include, freeing the names for Arduino.
//
// Use this instead of <xc.h> in any translation unit that also names an Arduino
// object/type that could clash.
//
// NOTE: xc.h also declares SFR *variables* SPI1/SPI2/SPI3 (generic struct
// accessors) and self-#defines them, so those names cannot be reclaimed by a
// macro here — which is why the SPI library's extra buses are named SPI_2/SPI_3
// rather than SPI1/SPI2 (see SPI.h).
//
#define SPI   _xc_generic_SPI_t
#define UART  _xc_generic_UART_t
#define PSPI  _xc_generic_PSPI_t
#define PUART _xc_generic_PUART_t
#include <xc.h>
#undef SPI
#undef UART
#undef PSPI
#undef PUART

#endif /* XC_COMPAT_H */
