#ifndef __PWMBLOCK__
#define __PWMBLOCK__

#include "BaseDevice.h"

using namespace ZPUino;

class PWMBlock_class: public BaseDevice
{
public:
    PWMBlock_class();
    void begin();
    void set(uint16_t index, uint8_t value);
private:
    uint16_t outputs;
    uint32_t base;
};

#endif
