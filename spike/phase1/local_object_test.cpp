// local_object_test.cpp — C++ with LOCAL (stack) objects only, no global
// constructors (so no .ctors-before-main startup path). Isolates whether plain
// C++ classes/objects/virtual dispatch work on a target (e.g. dsPIC33A) when the
// global-ctor mechanism is in question.
//
// PASS: R[0]=2 (derived virtual), R[1]=1 (base virtual), R[2]=99 (state),
//       R[3]=0xABCD (signature).

volatile int R[4];

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
    Derived d;               // local object, constructed at runtime in main
    Base*   p = &d;
    R[0] = p->f();           // virtual dispatch through base ptr -> Derived::f = 2
    Base    b(7);            // second local object
    R[1] = b.f();            // -> Base::f = 1
    R[2] = d.tag;            // 99 (set by Derived()->Base(99))
    R[3] = 0xABCD;
    done();
    while (1) { }
    return 0;
}
