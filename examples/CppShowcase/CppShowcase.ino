/*
 * CppShowcase - dspicArduino
 *
 * Shows REAL C++ running on a dsPIC33CK: classes, constructors, encapsulation,
 * an abstract base class, and virtual methods (polymorphism / vtable dispatch) —
 * none of which Microchip's stock compiler enables.
 *
 * Board: DM330030 (dsPIC33CK256MP508)
 *   Button : RE7  (external pull-up -> pressed = LOW)
 *   LEDs   : RE5, RE6
 *
 * Each button press cycles to the next LED pattern. The current pattern is
 * chosen through a base-class pointer, so the call `pattern->step(...)` is a
 * virtual dispatch resolved at run time on the dsPIC.
 */

// ---- a pin wrapped in an object -------------------------------------------
class Led {
    uint8_t _pin;
public:
    explicit Led(uint8_t pin) : _pin(pin) {}
    void begin()          { pinMode(_pin, OUTPUT); off(); }
    void on()             { digitalWrite(_pin, HIGH); }
    void off()            { digitalWrite(_pin, LOW); }
    void set(bool state)  { digitalWrite(_pin, state ? HIGH : LOW); }
};

// ---- an edge-detecting button (external pull-up: released=HIGH, pressed=LOW) -
class Button {
    uint8_t       _pin;
    bool          _down;       // true while currently pressed (LOW)
    unsigned long _stamp;
public:
    explicit Button(uint8_t pin) : _pin(pin), _down(false), _stamp(0) {}
    void begin() { pinMode(_pin, INPUT); _down = false; }

    // True once per press (released->pressed edge), with a light debounce that
    // ignores repeats within 40 ms of the last accepted press.
    bool wasPressed() {
        bool down = (digitalRead(_pin) == LOW);
        bool edge = false;
        if (down && !_down && (millis() - _stamp > 40)) {
            edge = true;
            _stamp = millis();
        }
        _down = down;
        return edge;
    }
};

// ---- abstract pattern: virtual methods => polymorphism on dsPIC -----------
class Pattern {
public:
    virtual ~Pattern() {}
    virtual const char *name() const = 0;
    virtual void step(Led &a, Led &b, unsigned long t) = 0;   // pure virtual
};

class BothOff : public Pattern {
public:
    const char *name() const { return "both off"; }
    void step(Led &a, Led &b, unsigned long)        { a.off(); b.off(); }
};

class BothOn : public Pattern {
public:
    const char *name() const { return "both on"; }
    void step(Led &a, Led &b, unsigned long)        { a.on(); b.on(); }
};

class Alternate : public Pattern {
public:
    const char *name() const { return "alternate"; }
    void step(Led &a, Led &b, unsigned long t) {
        bool phase = (t / 250) & 1;
        a.set(phase); b.set(!phase);
    }
};

class BlinkTogether : public Pattern {
public:
    const char *name() const { return "blink together"; }
    void step(Led &a, Led &b, unsigned long t) {
        bool phase = (t / 250) & 1;
        a.set(phase); b.set(phase);
    }
};

// ---- objects -------------------------------------------------------------
Led    led5(RE5);
Led    led6(RE6);
Button button(RE7);

BothOff       m0;
BothOn        m1;
Alternate     m2;
BlinkTogether m3;
Pattern *patterns[] = { &m0, &m1, &m2, &m3 };   // array of base-class pointers
const int     N = sizeof(patterns) / sizeof(patterns[0]);
int           current = 0;

void setup() {
    Serial.begin(9600);
    led5.begin();
    led6.begin();
    button.begin();
    Serial.println("CppShowcase: press RE7 to cycle LED patterns");
    Serial.print("pattern: ");
    Serial.println(patterns[current]->name());
}

void loop() {
    if (button.wasPressed()) {
        current = (current + 1) % N;
        Serial.print("pattern: ");
        Serial.println(patterns[current]->name());   // virtual call
    }
    patterns[current]->step(led5, led6, millis());    // virtual dispatch
}
