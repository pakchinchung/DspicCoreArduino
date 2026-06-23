// HardwareSerial - COMPILE-SAFE STUB (Phase 4 will implement real UART).
//
// The dsPIC33CK "new UART" and the dsPIC33A UART have different status/control
// register layouts than the legacy dsPIC33E/F UART, and PPS pin routing is
// board-specific. Rather than hard-code one family's registers, this stub lets
// sketches that reference Serial compile and run on every supported device;
// it simply produces no UART output yet. Real UART1/UART2 (BRG setup, PPS, ring
// buffers + RX/TX ISRs) lands in a later phase.

#include "HardwareSerial.h"
#include "Arduino.h"

void   HardwareSerial::begin(unsigned long baud) { (void)baud; }
void   HardwareSerial::end(void)                 { }
int    HardwareSerial::available(void)           { return 0; }
int    HardwareSerial::peek(void)                { return -1; }
int    HardwareSerial::read(void)                { return -1; }
size_t HardwareSerial::write(uint8_t c)          { (void)c; return 1; }

HardwareSerial Serial;    // UART1 (stub)
HardwareSerial Serial2;   // UART2 (stub)
