#ifndef __PANEL_H__
#define __PANEL_H__

//#include "dualpanel.h"

#if 1

#include "Adafruit_GFX.h"
#include <BaseDevice.h>
#include <Arduino.h>

//#define WIDTH 32
//#define HEIGHT 32
#define NUMPIXELS (96*32)

extern const unsigned char linearTable[256];

using namespace ZPUino;

class RGBPanel_class: public Adafruit_GFX_32, public BaseDevice
{
public:
    RGBPanel_class(): Adafruit_GFX_32(), BaseDevice(1)
    {
    }
    unsigned STRIDE;

    void begin()
    {
        if (deviceBegin(0x08, 0x20)==0) {
            // Read in width and weight
            Serial.println("device detected");
            unsigned sizeinfo = REG(1);
            unsigned otherinfo = REG(0);
            Serial.println(sizeinfo>>16);
            Serial.println(sizeinfo &0xffff);
            Adafruit_GFX_32::begin(sizeinfo>>16, sizeinfo &0xffff);
            printf("Panel size: %dx%d\n",sizeinfo>>16, sizeinfo &0xffff);
            printf("A Panel size: %dx%d\n", WIDTH, HEIGHT);
            printf("Other info: %dx%d\n",otherinfo>>16, otherinfo &0xffff);
            STRIDE = (otherinfo & 0xFFFF);
#if 0
            for (int i=0;i <96;i++) {
                REG((128*32)+i) = 0xff00ff;
                REG((128*32)+128+i) = 0x00ffff;
                delay(500);
            }
#endif
#if 1
            for (int i=0;i <4;i++) {
                clear(0xffffff);
                apply();
                delay(500);
                clear(0x000000);
                apply();
                delay(500);
            }
#endif
        }
    }
    virtual void drawPixel(int x, int y, unsigned int v)
    {
        if (x<WIDTH && y<HEIGHT)
            screen[x+(y*STRIDE)]=v;
    }
    virtual unsigned readPixel(int x, int y)
    {
        if (x<WIDTH && y<HEIGHT)
            return screen[x+(y*STRIDE)];
        return 0;
    }

    void clear(unsigned int color=0)
    {
        int y,x;
        for (y=0;y<HEIGHT;y++) {
            for (x=0; x<WIDTH;x++) {
                screen[(y*WIDTH)+x] = color;
            }
        }
    }

    void test()
    {
    }

    bool applyEnabled;

    void setApplyEnabled(bool e) {
        applyEnabled=e;
    }

    // 4096.
    void apply()
    {
        int x,y;
        unsigned offset = 32*128;
        unsigned soff = offset;
        unsigned int *pix=&screen[0];

        for (y=0;y<HEIGHT;y++) {

            for (x=0;x<WIDTH;x++) {
                unsigned int v = *pix;
                REG(soff+x) = v;
                pix++;
            }
            soff+=STRIDE;
        }
    }


    static inline unsigned computeOverlay(const unsigned int *screen, const unsigned int *overlay)
    {
        const unsigned char *sourceptr = (const unsigned char *)overlay;
        const unsigned char *destptr = (const unsigned char *)screen;
        unsigned int v = 0;
        unsigned char *vptr = (unsigned char *)&v;

        int z;
        int alpha = sourceptr[0];
        for (z=1;z<4;z++) {
            int dst = destptr[z];
            int src = sourceptr[z];
            int fixup = (alpha*(src-dst))/256;
            dst += fixup;
            vptr[z] = dst&0xff;
        }
        return v;
    }

    void applyWithOverlay(const unsigned int *overlay)
    {
#if 0
        // TODO for simple panel
        int i,z;
        unsigned *fbptr=&screen[0];
        unsigned offset = 32*128;
        unsigned linecomp = 128-96;

        for (i=0;i<32;i++) {
            for (z=0;z<96;z++) {
                unsigned int v = computeOverlay(fbptr, overlay);
                p0.REG(offset) = v;
                v = computeOverlay(&fbptr[96*32], &overlay[96*32]);
                p1.REG(offset) = v;
                fbptr++;
                overlay++;
                offset++;
            }
            /* Move to next line, hw */
            offset+=linecomp;
        }
#endif
    }



    inline void setPixel(int x, int y, int r, int g, int b)
    {
        unsigned char c0 = linearTable[r&0xff];
        unsigned char c1 = linearTable[g&0xff];
        unsigned char c2 = linearTable[b&0xff];
        unsigned int v = c0 + (c1<<8) + (c2<<16);
        screen[x+(y*WIDTH)]=v;
    }

    inline void setPixelRaw(int x, int y, int v)
    {
        screen[x+(y*WIDTH)]=v;
    }

    void setRawImage(const unsigned int *base)
    {
        int i;
        for (i=0;i<NUMPIXELS; i++) {
            // Load and parse each of the RGB.
            unsigned int v = base[i];
            // Apply fix up
            unsigned char c0 = v&0xff;
            unsigned char c1 = (v>>8)&0xff;
            unsigned char c2 = (v>>16)&0xff;
            v = c0 + (c1<<8) + (c2<<16);
            screen[i] = v;
        }
    }
    void setImage(const unsigned int *base)
    {
        int i;
        for (i=0;i<NUMPIXELS; i++) {
            // Load and parse each of the RGB.
            unsigned int v = base[i];
            // Apply fix up
            unsigned char c0 = linearTable[v&0xff];
            unsigned char c1 = linearTable[(v>>8)&0xff];
            unsigned char c2 = linearTable[(v>>16)&0xff];
            v = c0 + (c1<<8) + (c2<<16);
            screen[i] = v;
            //screen[i+(32*32)] = v;
        }
    }

    static unsigned fix32(unsigned int v)
    {
        unsigned char c0 = linearTable[v&0xff];
        unsigned char c1 = linearTable[(v>>8)&0xff];
        unsigned char c2 = linearTable[(v>>16)&0xff];
        v = c0 + (c1<<8) + (c2<<16);
    }

    unsigned int screen[NUMPIXELS];
    unsigned int *getScreen() {
        return &screen[0];
    }
};
extern RGBPanel_class RGBPanel;

#endif
#endif
