#ifndef Stream_h
#define Stream_h

#include "Print.h"

class Stream : public Print
{
public:
    virtual int  available(void) = 0;
    virtual int  read(void)      = 0;
    virtual int  peek(void)      = 0;

    // TODO Phase 4: readBytes, readString, setTimeout, find, etc.
};

#endif
