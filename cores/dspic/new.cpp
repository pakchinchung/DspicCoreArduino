// C++ heap operators for dspicArduino, backed by the XC-DSC C library.
// Standard Arduino C++ (virtual destructors, `new`/`delete`, many libraries)
// needs these; without them the linker reports undefined `operator new/delete`.

#include <stdlib.h>

void *operator new(size_t n)        { return malloc(n); }
void *operator new[](size_t n)      { return malloc(n); }
void  operator delete(void *p)      { free(p); }
void  operator delete[](void *p)    { free(p); }

// Sized deallocation (C++14): emitted by deleting destructors.
void  operator delete(void *p, size_t)   { free(p); }
void  operator delete[](void *p, size_t) { free(p); }
