/*
 * WireLoopback - dspicArduino
 *
 * I2C loopback on one chip: Wire (I2C1 master) talks to a minimal I2C2 echo
 * slave (implemented below). The master writes a byte to the slave and reads it
 * back; the readback should equal what was sent.
 *
 *   *** WIRING (jumpers + pull-ups) ***
 *     Wire (I2C1) defaults to its ALTERNATE pins on this board:
 *     SCL: RC9 (ASCL1) <--> RB6 (SCL2)
 *     SDA: RC8 (ASDA1) <--> RB5 (SDA2)
 *     Pull-ups: ~4.7k from SCL line to 3.3V, and from SDA line to 3.3V.
 *
 * Output on Serial @ 9600.  (The slave is a first cut — report results.)
 */
#include <Wire.h>

#define SLAVE_ADDR 0x42

// ---- minimal I2C2 echo slave (register-level; not part of the core yet) ----
#include <xc.h>
static volatile uint8_t slaveReg = 0;

extern "C" void __attribute__((interrupt, no_auto_psv)) _SI2C2Interrupt(void)
{
    uint8_t dummy;
    if (I2C2STATbits.D_A == 0) {            // address phase
        if (I2C2STATbits.R_W) {             // master wants to read -> send our reg
            I2C2TRN = slaveReg;
        } else {
            dummy = I2C2RCV; (void)dummy;   // read address out of the buffer
        }
    } else {                                // data phase
        if (I2C2STATbits.R_W == 0) {        // master is writing -> capture byte
            slaveReg = (uint8_t)I2C2RCV;
        } else {                            // master reading more -> resend
            I2C2TRN = slaveReg;
        }
    }
    I2C2CONLbits.SCLREL = 1;                 // release SCL clock stretch
    _SI2C2IF = 0;
}

static void slaveBegin(uint8_t addr)
{
    I2C2CONL = 0;
    I2C2ADD  = addr;                         // 7-bit slave address
    _SI2C2IF = 0;
    _SI2C2IE = 1;                            // enable slave interrupt
    I2C2CONLbits.I2CEN = 1;                  // enable I2C2 (takes SCL2/SDA2)
}

void setup() {
    Serial.begin(9600);
    slaveBegin(SLAVE_ADDR);                  // I2C2 slave
    Wire.begin();                            // I2C1 master
    Wire.setClock(100000);
}

void loop() {
    const uint8_t tv[] = { 0x11, 0xA5, 0x5A, 0xFF, 0x01 };
    int pass = 0;
    for (uint8_t i = 0; i < 5; i++) {
        Wire.beginTransmission(SLAVE_ADDR);  // write a byte to the slave
        Wire.write(tv[i]);
        uint8_t st = Wire.endTransmission();

        uint8_t got = 0;
        if (Wire.requestFrom((uint8_t)SLAVE_ADDR, (uint8_t)1) == 1) got = Wire.read();

        bool ok = (st == 0) && (got == tv[i]);
        if (ok) pass++;
        Serial.print("sent 0x"); Serial.print(tv[i], HEX);
        Serial.print(" txStatus="); Serial.print(st);
        Serial.print(" readback 0x"); Serial.print(got, HEX);
        Serial.println(ok ? "  ok" : "  MISMATCH");
    }
    Serial.print("I2C loopback: "); Serial.print(pass); Serial.println("/5");
    Serial.println();
    delay(3000);
}
