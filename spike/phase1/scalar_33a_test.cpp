// scalar_33a_test.cpp — C++ OOP test using plain scalar globals (not an array),
// because on dsPIC33A the simulator (mdb) reads simple globals reliably but
// returns null for C++ array elements. Same OOP coverage as the array test.
//
// PASS: rv_poly==2 (derived virtual), rv_tag==99 (state), rv_base==1 (base
//       virtual), rv_sig==0xABCD (ran to completion).
volatile int rv_poly;
volatile int rv_tag;
volatile int rv_base;
volatile int rv_sig;

struct Base {
    int tag;
    Base(int t) : tag(t) {}
    virtual int f() { return 1; }
};
struct Derived : public Base {
    Derived() : Base(99) {}
    int f() override { return 2; }
};

extern "C" void done(void) __attribute__((noinline));
extern "C" void done(void) { asm volatile(""); }

int main(void)
{
    Derived d;
    Base*   p = &d;
    rv_poly = p->f();    // virtual dispatch -> Derived::f = 2
    rv_tag  = d.tag;     // 99
    Base b(7);
    rv_base = b.f();     // Base::f = 1
    rv_sig  = 0xABCD;
    done();
    while (1) { }
    return 0;
}
