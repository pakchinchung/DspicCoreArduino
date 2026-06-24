# Phase 2+3 requirements (from user) — captured 2026-06-23

## Target board
- **Microchip DM330030** (confirm exact MCU on the PIM; assuming dsPIC33CK256MP508,
  which has all the pins below and matches what we verified in Phase 1).
- User will flash/test the **blink sketch on this real hardware**.

## Board pinout (start here)
- **Yellow user LEDs:** RE6, RE5
- **RGB LED:** RE15 = Red, RE14 = Green, RE13 = Blue
- **User buttons:** RE7, RE8, RE9 — all have **external pull-up resistors**
  (so configure as INPUT, not INPUT_PULLUP; pressed = reads LOW)

## Final goal
- **Custom board support:** user picks their MCU and defines pins by name, e.g.
  `#define LED_USER RE7`. Pin-by-native-name (RExx) is the primary model;
  Arduino-style numbers can layer on top.

## Open question to resolve (user asked)
- Should we support **soft (bit-banged) I²C, UART, SPI**, like ESP32 does?
  - Working recommendation (pending user decision): hardware peripherals FIRST,
    using dsPIC **PPS** to give hardware UART/SPI ESP32-like pin flexibility;
    add soft I²C / SoftwareSerial / bit-bang SPI as follow-ons (I²C is fixed-pin
    so soft-I²C is the most useful soft variant).

## Compiler / environment
- C++ compiler is **Linux-hosted (WSL)**; backed up to `C:\dspic-cpp-toolchain`.
  Compiling Phase 2 requires WSL running. A Windows-native (no-WSL) compiler is a
  separate cross-build effort.

## Status when paused
- Started `spike/phase2/blink_button_test.cpp` (pin-by-name GPIO model + blink/
  button logic) — NOT yet compiled/verified. Paused at user's request for questions.
