/* ctor_test.c — proves the C++ static-constructor mechanism on dsPIC,
 * using ONLY the stock C compiler. __attribute__((constructor)) emits into
 * the same .ctors table that C++ global object constructors use, and
 * Microchip's crt0 calls __ctor (which walks that table) before main().
 *
 * PASS criteria (inspect on the MPLAB X simulator):
 *   after reaching main(), global 'main_saw_ctor' == 0xC0DE
 *   meaning the constructor ran BEFORE main, exactly like a C++ global object.
 */
volatile int ctor_ran      = 0;
volatile int main_saw_ctor = 0;

/* This is what the C++ front-end would generate for a global object's ctor. */
__attribute__((constructor))
void my_global_ctor(void)
{
    ctor_ran = 0xC0DE;
}

int main(void)
{
    main_saw_ctor = ctor_ran;   /* 0xC0DE proves ctor ran first */
    while (1) { }
    return 0;
}
