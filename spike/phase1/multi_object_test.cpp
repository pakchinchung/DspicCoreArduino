// multi_object_test.cpp — richer C++ proof for dsPIC.
//
// Demonstrates real OOP working on the chip:
//   - a class with state + a constructor
//   - MULTIPLE objects (independent instances with their own state)
//   - inheritance + virtual methods (polymorphism through base pointers)
//   - global objects constructed by the .ctors mechanism before main()
//
// No new/delete, no exceptions/RTTI, trivial destructors -> minimal C++ runtime,
// so it links with the stock dsPIC startup (crt0 runs .ctors) + device libs.
//
// PASS (read these globals on the simulator after reaching the while(1) loop):
//   r_a_start == 10     r_a_next == 11      (object a: Counter(10), next->11)
//   r_b_start == 100    r_b_next == 102     (object b: Counter(100), step 2)
//   r_c_val   == 7      (object c: Stepper(0, step 7) after one next)
//   r_poly_sum == 11 + 102 + 7 ... see below
//   r_count   == 3      (number of objects iterated polymorphically)

class Counter {
protected:
    int v;
public:
    Counter(int start) : v(start) {}
    virtual int next() { return ++v; }       // base: +1
    int value() const { return v; }
};

class StepCounter : public Counter {
    int step;
public:
    StepCounter(int start, int s) : Counter(start), step(s) {}
    int next() override { v += step; return v; }   // derived: +step (polymorphic)
};

// --- three independent global objects (constructed via .ctors before main) ---
static Counter      a(10);
static StepCounter  b(100, 2);
static StepCounter  c(0, 7);

// --- observable results in ONE array (single symbol -> clean simulator reads) ---
//   R[0]=a start(10)  R[1]=a.next(11)  R[2]=b start(100) R[3]=b.next(102)
//   R[4]=c value(7)   R[5]=poly sum(123) R[6]=count(3)   R[7]=signature(0xABCD)
volatile int R[8];

// Reliable breakpoint target (C linkage, not inlined): break here once all
// results are computed, then read the globals on the simulator.
extern "C" void done(void) __attribute__((noinline));
extern "C" void done(void) { asm volatile(""); }

int main()
{
    // Per-object state is independent:
    R[0] = a.value();      // 10
    R[1] = a.next();       // 11  (Counter::next, +1)
    R[2] = b.value();      // 100
    R[3] = b.next();       // 102 (StepCounter::next, +2)

    // Polymorphism: iterate a heterogeneous set through base pointers.
    Counter* objs[3] = { &a, &b, &c };
    int sum = 0;
    int n = 0;
    for (int i = 0; i < 3; ++i) {
        sum += objs[i]->next();   // virtual dispatch: a->+1, b->+2, c->+7
        ++n;
    }
    // After the loop: a=12, b=104, c=7  -> sum = 12 + 104 + 7 = 123
    R[4] = c.value();      // 7   (assign in index order so reads are race-safe)
    R[5] = sum;            // 123
    R[6] = n;              // 3
    R[7] = 0xABCD;         // signature: proves we reached the end with all set

    done();                // <-- simulator breakpoint target (all results set)
    while (1) { }
    return 0;
}
