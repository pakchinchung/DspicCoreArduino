#ifndef WString_h
#define WString_h

// Minimal String stub — enough to satisfy includes.
// TODO Phase 4: implement dynamic String class fully.

#include <string.h>
#include <stdint.h>

class String
{
public:
    String(const char *s = "") : _buf(s) {}
    const char *c_str() const { return _buf; }
    unsigned int length() const { return strlen(_buf); }

private:
    const char *_buf;
};

#endif
