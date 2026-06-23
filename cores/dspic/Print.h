#ifndef Print_h
#define Print_h

#include <stdint.h>
#include <stddef.h>
#include "WString.h"

#define DEC  10
#define HEX  16
#define OCT   8
#define BIN   2

class Print
{
public:
    virtual size_t write(uint8_t c) = 0;
    virtual size_t write(const uint8_t *buf, size_t len);

    size_t print(const char str[]);
    size_t print(char c);
    size_t print(unsigned char n, int base = DEC);
    size_t print(int n, int base = DEC);
    size_t print(unsigned int n, int base = DEC);
    size_t print(long n, int base = DEC);
    size_t print(unsigned long n, int base = DEC);
    size_t print(double n, int digits = 2);
    size_t print(const String &s);

    size_t println(const char str[]);
    size_t println(char c);
    size_t println(unsigned char n, int base = DEC);
    size_t println(int n, int base = DEC);
    size_t println(unsigned int n, int base = DEC);
    size_t println(long n, int base = DEC);
    size_t println(unsigned long n, int base = DEC);
    size_t println(double n, int digits = 2);
    size_t println(const String &s);
    size_t println(void);

private:
    size_t printNumber(unsigned long n, uint8_t base);
};

#endif
