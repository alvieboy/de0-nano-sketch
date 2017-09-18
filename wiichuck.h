#ifndef __WIICHUCK_H__
#define __WIICHUCK_H__

#include "i2c.h"

using namespace ZPUino;

#define BUTTON_Z 1
#define BUTTON_C 2
#define BUTTON_ZC 3

#define EVENT_NONE    0
#define EVENT_PRESS   1
#define EVENT_RELEASE 2

class WIIChuck_class
{
public:
    WIIChuck_class(I2C_class &i2c): i2c_instance(i2c) {}
    I2C_class &i2c_instance;

    void begin();
    void prstatus();
    void update(void (*idle)(void)=NULL);

    int init_nunchuck();
    int send_zero(void (*idle)(void)=NULL);

    unsigned char nunchuk_decode_byte (unsigned char x);
    void nunchuck_update_data(unsigned char *nunchuck_buf);

    int getJoyX() const { return joy_x_axis; }
    int getJoyY() const { return joy_y_axis; }
    int getZButton() const { return z_button; }
    int getZEvent() const { return z_button ^ p_z_button ? ( z_button ? EVENT_PRESS : EVENT_RELEASE ) : EVENT_NONE; }
    int getCButton() const { return c_button; }
    int getCEvent() const { return c_button ^ p_c_button ? ( c_button ? EVENT_PRESS : EVENT_RELEASE ) : EVENT_NONE; }
    int getButtons() const { return (c_button<<1)+z_button; }

    int getAccelX() const { return accel_x_axis; }
    int getAccelY() const { return accel_y_axis; }
    int getAccelZ() const { return accel_z_axis; }
    int joy_x_axis;
    int joy_y_axis;
    int accel_x_axis;
    int accel_y_axis;
    int accel_z_axis;

    int z_button;
    int c_button;

    int p_z_button;
    int p_c_button;

};

#endif
