// blink_button_test.cpp — Phase 2+3 GPIO proof for the DM330030 board pinout.
//
// Demonstrates the pin-by-NAME model (digitalWrite(RE6, ...)) that custom-board
// support is built on, plus pinMode/digitalRead, exercised on the dsPIC33CK and
// verified on the MPLAB X simulator.
//
// Board (DM330030) pinout given by the user:
//   Yellow LEDs : RE6, RE5            (driven HIGH = on, assumed active-high)
//   RGB LED     : RE15(R) RE14(G) RE13(B)
//   Buttons     : RE7, RE8, RE9       (EXTERNAL pull-ups -> INPUT; pressed = LOW)
//
// Sim verification (no peripherals needed, just GPIO registers):
//   - after setup+loop body, LATE bit6 should reflect the LED state we drove
//   - we read the button on RE7 from PORTE; the sim injects PORTE to emulate it
//   - results mirrored into scalar globals for easy reading

#include <xc.h>
#include <stdint.h>

// ---- pin-by-name model -----------------------------------------------------
// A pin id encodes its port (A=0,B=1,C=2,D=3,E=4,...) and bit: (port<<4)|bit.
#define PIN(port, bit)  (((port) << 4) | (bit))
enum { _PA=0,_PB,_PC,_PD,_PE,_PF,_PG };

// Only the pins this board uses (full set lives in the variant header later):
#define RE5  PIN(_PE,5)
#define RE6  PIN(_PE,6)
#define RE7  PIN(_PE,7)
#define RE8  PIN(_PE,8)
#define RE9  PIN(_PE,9)
#define RE13 PIN(_PE,13)
#define RE14 PIN(_PE,14)
#define RE15 PIN(_PE,15)

#define OUTPUT 1
#define INPUT  0
#define HIGH   1
#define LOW    0

// Per-port register pointers, indexed by port number.
struct PortRegs { volatile uint16_t *tris, *lat, *port, *ansel; };
static const PortRegs g_ports[] = {
    { &TRISA, &LATA, &PORTA, &ANSELA },
    { &TRISB, &LATB, &PORTB, &ANSELB },
    { &TRISC, &LATC, &PORTC, &ANSELC },
    { &TRISD, &LATD, &PORTD, &ANSELD },
    { &TRISE, &LATE, &PORTE, &ANSELE },
};

static inline void pinMode(uint16_t pin, uint8_t mode) {
    const PortRegs *p = &g_ports[pin >> 4];
    uint16_t m = (uint16_t)(1u << (pin & 0xF));
    *p->ansel &= ~m;                 // digital (not analog)
    if (mode == OUTPUT) *p->tris &= ~m;
    else                *p->tris |=  m;
}
static inline void digitalWrite(uint16_t pin, uint8_t val) {
    const PortRegs *p = &g_ports[pin >> 4];
    uint16_t m = (uint16_t)(1u << (pin & 0xF));
    if (val) *p->lat |= m; else *p->lat &= ~m;
}
static inline int digitalRead(uint16_t pin) {
    const PortRegs *p = &g_ports[pin >> 4];
    return (*p->port & (uint16_t)(1u << (pin & 0xF))) ? HIGH : LOW;
}

// ---- board aliases (this is what a user would put in their board file) -----
#define LED_USER  RE6
#define LED_R     RE15
#define BTN_USER  RE7     // external pull-up: idle HIGH, pressed LOW

// ---- observable results for the simulator ----------------------------------
volatile int r_led_on;     // LATE bit6 after we drive LED on
volatile int r_led_off;    // LATE bit6 after we drive LED off
volatile int r_btn_raw;    // digitalRead(BTN_USER)
volatile int r_led_follows;// LED state after "if button pressed, LED on"
volatile int r_sig;

extern "C" void done(void) __attribute__((noinline));
extern "C" void done(void) { asm volatile(""); }

int main(void)
{
    pinMode(LED_USER, OUTPUT);
    pinMode(LED_R,    OUTPUT);
    pinMode(BTN_USER, INPUT);

    digitalWrite(LED_USER, HIGH);
    r_led_on  = (LATE >> 6) & 1;        // expect 1
    digitalWrite(LED_USER, LOW);
    r_led_off = (LATE >> 6) & 1;        // expect 0

    // Read the button (sim injects PORTE bit7). Pressed (LOW) -> light the LED.
    r_btn_raw = digitalRead(BTN_USER);
    if (digitalRead(BTN_USER) == LOW) digitalWrite(LED_USER, HIGH);
    else                              digitalWrite(LED_USER, LOW);
    r_led_follows = (LATE >> 6) & 1;    // = 1 if button was pressed (PORTE.7 low)

    r_sig = 0xABCD;
    done();
    while (1) { }
    return 0;
}
