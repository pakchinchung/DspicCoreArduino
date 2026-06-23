// TODO Phase 4: implement Print methods.
// All overloads funnel through write(uint8_t) which each subclass provides.
#include "Print.h"
#include <string.h>
#include <stdio.h>

size_t Print::write(const uint8_t *buf, size_t len)
{
    size_t n = 0;
    while (len--) n += write(*buf++);
    return n;
}

size_t Print::print(const char str[])
{
    return write((const uint8_t *)str, strlen(str));
}

size_t Print::print(char c)         { return write((uint8_t)c); }
size_t Print::println(void)         { return write('\r') + write('\n'); }

size_t Print::println(const char s[]) { return print(s) + println(); }

size_t Print::printNumber(unsigned long n, uint8_t base)
{
    char buf[8 * sizeof(long) + 1];
    char *p = buf + sizeof(buf) - 1;
    *p = '\0';
    do {
        unsigned long rem = n % base;
        *--p = rem < 10 ? '0' + rem : 'A' + rem - 10;
        n /= base;
    } while (n);
    return write((const uint8_t *)p, strlen(p));
}

size_t Print::print(int n, int base)          { return n < 0 ? write('-') + printNumber((unsigned long)-n, base) : printNumber(n, base); }
size_t Print::print(unsigned int n, int base) { return printNumber(n, base); }
size_t Print::print(long n, int base)         { return n < 0 ? write('-') + printNumber((unsigned long)-n, base) : printNumber(n, base); }
size_t Print::print(unsigned long n, int base){ return printNumber(n, base); }
size_t Print::print(unsigned char n, int base){ return printNumber(n, base); }

size_t Print::println(int n, int base)          { return print(n, base) + println(); }
size_t Print::println(unsigned int n, int base) { return print(n, base) + println(); }
size_t Print::println(long n, int base)         { return print(n, base) + println(); }
size_t Print::println(unsigned long n, int base){ return print(n, base) + println(); }
size_t Print::println(unsigned char n, int base){ return print(n, base) + println(); }

size_t Print::print(double n, int digits)
{
    char buf[32];
    // dtostrf not available on dsPIC; use snprintf from XC-DSC libc.
    snprintf(buf, sizeof(buf), "%.*f", digits, n);
    return print(buf);
}
size_t Print::println(double n, int d) { return print(n, d) + println(); }

size_t Print::print(const String &s)   { return print(s.c_str()); }
size_t Print::println(const String &s) { return println(s.c_str()); }
size_t Print::println(char c)          { return print(c) + println(); }
