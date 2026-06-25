// cpp_test.cpp — the REAL C++ acceptance test for Phase 1.
// Run this once cc1plus is built. It mirrors ctor_test.c but in genuine C++:
// a global object whose constructor runs before main, plus a virtual call.
//
// PASS (on the simulator): main_saw == 0xC0DE  AND  vcall == 42
//
// Build (after cc1plus exists):
//   xc16pp-compile -lang=cpp -mcu=33CK256MP508 -c -O1 -fno-exceptions -fno-rtti \
//       cpp_test.cpp -o cpp_test.o
//   xc16pp-link -mcu=33CK256MP508 cpp_test.o -o cpp_test.elf

volatile int main_saw = 0;
volatile int vcall    = 0;

struct Base {
    virtual int value() { return 0; }   // virtual → vtable dispatch
};
struct Derived : Base {
    int value() override { return 42; }
};

struct Sentinel {
    Sentinel() { main_saw = 0xC0DE; }   // global ctor must run before main
};

static Sentinel  g_sentinel;            // global object with constructor
static Derived   g_obj;
static Base     *g_ptr = &g_obj;        // base pointer to derived

int main()
{
    vcall = g_ptr->value();             // expect 42 via vtable
    while (1) { }
    return 0;
}
