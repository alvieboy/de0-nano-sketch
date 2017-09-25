#include "i2c.h"

class I2C_class I2C;


int I2C_class::writeregister(uint8_t base, uint8_t address, uint8_t value)
{
    unsigned char r;
    if (I2C.start(base,0)!=0) {
        Serial.println("Cannot start i2c");
        return -1;
    }
    if (I2C.tx(address)<0)
        return -1;
    if (I2C.tx(value)<0)
        return -1;
    I2C.stop();
    delay(1);
    return 0;
}


int I2C_class::readregister(uint8_t base, uint8_t value)
{
    unsigned char r;

    if (I2C.start(base,0)!=0) {
        //    Serial.println("Error 1");
        return -1;
    }
    if (I2C.tx(value)<0)
        return -1;
    I2C.stop();

    delay(1);

    if (I2C.start(base,1)<0)
        return -1;

    if (I2C.rx(&r,1,true)<0)
        return -1;

    return (int)r;
}

int I2C_class::readregister16(uint8_t base, uint8_t value)
{
    uint8_t r[2];

    if (I2C.start(base,0)!=0) {
        //    Serial.println("Error 1");
        return -1;
    }
    if (I2C.tx(value)<0)
        return -1;
    I2C.stop();

    delay(1);

    if (I2C.start(base,1)<0)
        return -1;

    if (I2C.rx(&r[0],2,true)<0)
        return -1;

    return (int)(((unsigned)r[0]<<0) + ((unsigned)r[1]<<8));
}

int I2C_class::start(unsigned int address, int read, void (*idle)(void))
{
    I2CREG_TXR = (address<<1)|read;
    I2CREG_CR = _BV(I2C_STA) | _BV(I2C_WR);

    while (I2CREG_SR & _BV(I2C_TIP)) {
        if (idle)
            idle();
    }

    if ((I2CREG_SR & 0x80)==0)
        return 0;
    /* got ack ? */
    return -1;
}

int I2C_class::rx(unsigned char *target, int size, bool stop, void (*idle)(void))
{
    while (size--) {
        unsigned f = _BV(I2C_RD);

        if(size==0) {
            f|=_BV(I2C_ACK);
            if (stop)
                f|=_BV(I2C_STO);
        }
        I2CREG_CR = f;
        do {
            if (idle)
                idle();

            f=I2CREG_SR;
        } while ( f & _BV(I2C_TIP) );

        *target++=I2CREG_RXR;
        if (size==0)
            return 0;

        if ((I2CREG_SR & 0x80)!=0)   {
            //Serial.print("Err while remaining");
            //Serial.println(size);
            return -1;
        }
        //Serial.print("R ");
    }
    return 0;
}

int I2C_class::tx(unsigned byte, void (*idle)(void))
{
    I2CREG_TXR = byte;
    I2CREG_CR = _BV(I2C_WR);
    while (I2CREG_SR & 0x2) {
        if (idle)
            idle();
    }
    if ((I2CREG_SR & 0x80)==0x0)
        return 0;

    return -1;
}
