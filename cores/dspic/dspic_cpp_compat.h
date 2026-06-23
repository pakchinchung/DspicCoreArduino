/*
 * dspic_cpp_compat.h — C++ compatibility shim for Microchip dsPIC device headers.
 *
 * WHY THIS EXISTS
 *   GCC 8.3's C++ frontend leaves c_register_addr_space() an empty stub
 *   (gcc/cp/tree.c), so the pic30 named-address-space keywords (__prog__,
 *   __eds__, __psv__, ...) are never registered for C++. Including <xc.h> from
 *   C++ therefore fails to parse, e.g. "'__prog__' does not name a type".
 *
 * SCOPE (verified, not assumed) — target families dsPIC33CK and dsPIC33AK:
 *   Scanned the FULL <xc.h> include chain of every CK + AK device header in the
 *   installed DFPs plus the XC-DSC shared include/ dir:
 *     - CK : the ONLY address-space keyword used is __prog__ (~5800 uses), and
 *            EVERY one is on a declaration that already carries
 *            __attribute__((space(prog))). So neutralizing the bare keyword
 *            loses no placement semantics — the attribute still applies.
 *     - AK : uses NONE of these keywords (the pic30 backend deliberately does
 *            not register them for 33A devices), so this shim is a harmless
 *            no-op there.
 *
 * WHAT WE DO
 *   Neutralize ONLY __prog__ under C++. We intentionally do NOT blanket-define
 *   __eds__/__psv__/__pmp__/etc.: none of them appear in CK/AK headers, and
 *   leaving them undefined means hand-written C++ that uses one still produces a
 *   clear compile error (fail loud) rather than silently placing data in RAM.
 *
 * LIMITATION
 *   Hand-written C++ that uses __prog__ as a qualifier to place USER data in
 *   program space falls back to RAM. Use __attribute__((space(prog))) directly,
 *   or compile that translation unit as C. Full C++ named-address-space support
 *   (cp/parser.c grammar work) is a planned follow-up.
 *
 * This header is force-included (-include) ahead of <xc.h> by the build wrapper.
 */
#ifndef DSPIC_CPP_COMPAT_H
#define DSPIC_CPP_COMPAT_H

#ifdef __cplusplus
/* The only pic30 address-space keyword used by CK/AK device headers, always
   paired with __attribute__((space(prog))). Accepted + ignored in C++. */
#define __prog__
#endif /* __cplusplus */

#endif /* DSPIC_CPP_COMPAT_H */
