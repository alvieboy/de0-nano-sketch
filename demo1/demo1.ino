#include <SmallFS.h>
#include <zlib.h>
#include <png.h>
#include <PNGFile.h>
#include <Timer.h>
#include <SPI.h>

#include "pwmblock.h"
#include "i2c.h"
#include "decay.h"
#include "Adafruit_GFX.h"
#include "panelv2.h"
#include "wiichuck.h"
#include "tetris.h"
#include "font.h"
#include "samplingbuffer.h"
#include "FFT.h"
#include "window.h"

#define ACCEL_I2C_ADDRESS 0x1D
#define EEPROM_I2C_ADDRESS 0x50

#define ACCEL_REG_X 0x32
#define ACCEL_REG_Y 0x34
#define ACCEL_REG_Z 0x36

#define FPGA_PIN_DIP0        90
#define FPGA_PIN_DIP1        91
#define FPGA_PIN_DIP2        92
#define FPGA_PIN_DIP3        93

#define FFT_POINTS 1024
#define SAMPLE_BUFFER_SIZE FFT_POINTS

static PWMBlock_class PWMBlock;
static PanelV2_class RGBPanel;
static I2C_class chuck_i2c;
static WIIChuck_class WIIChuck(chuck_i2c);
static bool chuckPresent = false;

static SPIClass SPI_ext(1);
static SPIClass SPI_adc(2);

typedef fp32_16_16 fixed_t; // Fixed-point Q16

static Decay<8> led_decay;

static int lastindex=4;

typedef SamplingBuffer<SAMPLE_BUFFER_SIZE> mySamplingBuffer;

static mySamplingBuffer samplingBuffer;
static FFT_1024 myfft; // 1024-point FFT
static Window<1024> window_1024; // Hamming window 1024 points.



void read_accel()
{
    int16_t x;
    unsigned i2creg;

    unsigned pins;
    pins = digitalRead(FPGA_PIN_DIP1) << 1;
    pins += digitalRead(FPGA_PIN_DIP0);
    Serial.print("Pins ");
    Serial.println(pins);

    switch (pins) {
    case 0: i2creg = ACCEL_REG_X; break;
    case 1: i2creg = ACCEL_REG_Y; break;
    case 2: i2creg = ACCEL_REG_Z; break;
    default: i2creg = ACCEL_REG_X; break;
    }
    //I2C.writeregister(ACCEL_I2C_ADDRESS, 0x2D, (1<<3));

    x = I2C.readregister16(ACCEL_I2C_ADDRESS, i2creg);
    int i;

    x = x/32;
    int aidx = 4 - x;
    int idiff = aidx-lastindex;

    if (idiff>1) {
        aidx = lastindex+1;
    } else if (idiff<-1) {
        aidx = lastindex-1;
    }

    if (aidx<0) aidx=0;
    if (aidx>7) aidx=7;

    lastindex = aidx;


    for (i=0;i<8;i++) {
        led_decay.level(i, i==aidx? 255: 0);
    }

    for (i=0;i<8;i++) {
        uint8_t rv = linearTable[ (led_decay[i].asInt()) & 0xff ];
        PWMBlock.set(i,rv );
    }
}

static fixed_t gain = 1.0;

static void store_adc_sample(unsigned val)
{
    fixed_t fv;

    int sample = (int)(val & 0xFFF) -2048;
    // Current range: -2048 to 2047.

    fv.v = sample<<5; // Range: -1.0 to 0.9999  - Q16
    fv*=gain;
    fv.clampToUnity();
    // Clamp
    samplingBuffer.append( fv.v );

}

#define FPGA_PIN_ADC_CS      95
#define ADC_CHANNEL 0

static bool adc_read(void *data)
{

    digitalWrite(FPGA_PIN_ADC_CS, LOW);
    unsigned read = SPI_adc.transfer16( ADC_CHANNEL<<13 );
    digitalWrite(FPGA_PIN_ADC_CS, HIGH);
    // Toggle capture.
    store_adc_sample(read);
    return true;
}

void setup_adc()
{
    Timers.begin();
    Timers.periodicHz( 14400, &adc_read, NULL);
}


int wait_for_key()
{
    int i = 500;
    while (i--) {
        WIIChuck.update();
        if (WIIChuck.getZEvent()==EVENT_PRESS) {
            return 1;
        }
        if (WIIChuck.getCEvent()==EVENT_PRESS) {
            return 2;
        }
        delay(10);
    }
    return 0;
}



void setup()
{
    Serial.begin(115200);
    Serial.println("Start");
    I2C.begin(100000);
    WIIChuck.begin();
    if (WIIChuck.init_nunchuck()>=0) {
        chuckPresent=true;
    } else {
        chuckPresent=true;
        Serial.println("Could not initialize WII Nunchuck");
    }
    SmallFS.begin();
    font_init();

    PWMBlock.begin();
    RGBPanel.begin();
    RGBPanel.setApplyEnabled(true);

    SPI_ext.begin( MOSI(32), MISO(33), SCK(34) ); // 2nd instance

    SPI_adc.begin();  // 1st instance


    int i;
    for (i=0;i<8;i++) {
        RGBPanel.clear(i&1 ? 0xffffff:0);
        RGBPanel.apply();
    }
    RGBPanel.setPixelRaw(0,0,0xFFFFFF);
    RGBPanel.setPixelRaw(16,0,0xFFFFFF);
    RGBPanel.setPixelRaw(0,16,0xFFFFFF);
    RGBPanel.setPixelRaw(127,0,0xFFFFFF);
    RGBPanel.apply();
#if 1
    PNGFile png1, png2, png3;
    int r = 0;
    RGBPanel.setReverse(true);

    r+=png1.read("smallfs/terasic.png");
    r+=png2.read("smallfs/1.png");
    r+=png3.read("smallfs/2.png");

    if (r==0) {
        while (1) {
            Serial.println("Display one");
            png1.put32((uint32_t*)(&RGBPanel.getScreen()[0]),96);
            RGBPanel.apply();
            //while (wait_for_key()&2);

            if (wait_for_key()==2) break;
            //RGBPanel.fadeLeft(0,96);
            Serial.println("Display two");
            png2.put32((uint32_t*)(&RGBPanel.getScreen()[0]),96);
            RGBPanel.apply();

            //while (wait_for_key()&2);

            if (wait_for_key()==2) break;

            Serial.println("Display three");
            png3.put32((uint32_t*)(&RGBPanel.getScreen()[0]),96);
            RGBPanel.apply();

            //while (wait_for_key()&2);

            if (wait_for_key()==2) break;

        }
    }
#endif

    pinMode(FPGA_PIN_ACCEL_CS, OUTPUT);
    digitalWrite(FPGA_PIN_ACCEL_CS, HIGH);

    I2C.enable();

    do {
        int accel_id = I2C.readregister(ACCEL_I2C_ADDRESS, 0);
        if (accel_id<0) {
            Serial.println("Error reading accelerometer ID");
            break;
        }
        Serial.print("Accel ID (hex): ");
        Serial.println(accel_id, HEX);
        if (accel_id!=0xE5) {
            Serial.println("Invalid accelerometer ID");
            break;
        }
        // Power control
        I2C.writeregister(ACCEL_I2C_ADDRESS, 0x2D, (1<<3));
    } while (0);

    RGBPanel.setReverse(false);
    setup_game();

}


bool isDown() { return WIIChuck.getJoyY() < 64; }
bool isRotate() { return WIIChuck.getZButton(); }
bool isRight() { return WIIChuck.getJoyX() > 128+64; }
bool isLeft() { return WIIChuck.getJoyX() < 64; }

void loop()
{
    //read_accel();
    //delay(20);
    if (Serial.available()) {
        int i = Serial.read();
        if (i=='0') {
            Serial.println("Disable OE");
            RGBPanel.setOE(1);
        }
    }

    if (chuckPresent)
        WIIChuck.update();

    RGBPanel.clear();
    game_loop(&RGBPanel);
    RGBPanel.apply();
    read_accel();
    delay(5);
}


const unsigned char linearTable[256] = {
    0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 4, 4,
    4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 7,
    7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 10, 10, 10, 10, 11,
    11, 11, 12, 12, 12, 13, 13, 13, 14, 14, 15, 15, 15,
    16, 16, 17, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21,
    22, 22, 23, 23, 24, 24, 25, 25, 26, 26, 27, 28, 28,
    29, 29, 30, 31, 31, 32, 33, 33, 34, 35, 35, 36, 37,
    37, 38, 39, 40, 40, 41, 42, 43, 44, 44, 45, 46, 47,
    48, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 58,
    59, 60, 61, 62, 63, 65, 66, 67, 68, 69, 70, 71, 72,
    73, 74, 75, 77, 78, 79, 80, 81, 83, 84, 85, 86, 88,
    89, 90, 91, 93, 94, 95, 97, 98, 99, 101, 102, 104,
    105, 107, 108, 109, 111, 112, 114, 115, 117, 119,
    120, 122, 123, 125, 126, 128, 130, 131, 133, 135,
    136, 138, 140, 142, 143, 145, 147, 149, 151, 152,
    154, 156, 158, 160, 162, 164, 166, 168, 170, 171,
    173, 175, 178, 180, 182, 184, 186, 188, 190, 192,
    194, 196, 199, 201, 203, 205, 208, 210, 212, 214,
    217, 219, 221, 224, 226, 229, 231, 233, 236, 238,
    241, 243, 246, 248, 251, 253, 255
};
