#ifndef __i2c_h__
#define __i2c_h__

#include <Arduino.h>
#include <zpuino.h>

#define I2CREG_PRERlo REG(0)
#define I2CREG_PRERhi REG(1)
#define I2CREG_CTR REG(2)
#define I2CREG_TXR REG(3)
#define I2CREG_RXR REG(3)
#define I2CREG_CR REG(4)
#define I2CREG_SR REG(4)
#define I2CPRESCALE( x )  ( (CLK_FREQ/(5* x ) )-1 )

#define I2C_ENABLED 7
#define I2C_INTR_ENABLED 6

#define I2C_STA 7
#define I2C_STO 6
#define I2C_RD  5
#define I2C_WR  4
#define I2C_ACK 3
#define I2C_IACK 0

#define I2C_RXACK 7
#define I2C_BUSY 6
#define I2C_AL 5
#define I2C_TIP 1
#define I2C_IF 0

using namespace ZPUino;

class I2C_class: public BaseDevice
{
public:
    I2C_class(): BaseDevice() {
    }
    void begin(unsigned int speed) {
        if (deviceBegin(0x04, 0x01)==0) {
            unsigned pres = I2CPRESCALE(speed);
            I2CREG_CTR=0; // Disable all
            I2CREG_PRERhi = pres>>8;
            I2CREG_PRERlo = pres&0xff;
            /*printf("I2C at slot %d, instance %d, base register 0x%08x\r\n",
                   getSlot(), getInstance(), getBaseRegister());
              */
        } else {
            Serial.println("I2C device not found\r\n");
        }
    }

    void enable()
    {
        I2CREG_CTR = _BV(I2C_ENABLED);
    }

    unsigned int getStatus(){
        return I2CREG_SR;
    }

    int tx(unsigned byte, void (*idle)(void)=NULL);
    int rx(unsigned char *target, int size, bool stop, void (*idle)(void)=NULL);
    int start(unsigned int address, int read, void (*idle)(void)=NULL);

    void stop() {
        I2CREG_CR = _BV(I2C_STO);
    };

    int writeregister(uint8_t base, uint8_t address, uint8_t value);
    int readregister16(uint8_t base, uint8_t value);
    int readregister(uint8_t base, uint8_t address);


};


extern I2C_class I2C;

#endif
