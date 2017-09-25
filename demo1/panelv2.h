#ifndef __DUALPANEL_H__
#define __DUALPANEL_H__

#include "Adafruit_GFX.h"
#include <BaseDevice.h>
#include <Arduino.h>

#define RGBBASE IO_SLOT(9)
#ifdef INVERTXY
#define WIDTH 64
#define HEIGHT 128
#else

//#define WIDTH 128
//#define HEIGHT 64
#endif

//#define NUMPIXELS (WIDTH*32)

#ifdef __AVR__
 #include <avr/pgmspace.h>
#else
 #define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif
extern const unsigned char linearTable[256];
extern "C" const unsigned char font[];

using namespace ZPUino;

class PanelV2_class: public Adafruit_GFX_32, public BaseDevice
{
public:
    unsigned NUMPIXELS;
    bool hasdoublebuffer;
    unsigned framebuffersize;

    PanelV2_class(): Adafruit_GFX_32(), BaseDevice(1)
    {
        reverse=false;
    }
    bool begin(bool doubleBuffer=false)
    {
        unsigned char *ptr;
        
        unsigned sizeinfo;

        hasdoublebuffer=doubleBuffer;

        if (deviceBegin(0x88, 0x20)==0) {
            // Read in width and weight
            sizeinfo = REG(0);
        } else {
            Serial.println("Cannot find primary RGB panel");
            return false;
        }
        Serial.println("Found RGB panel controller v2 ");

        Serial.print(sizeinfo>>16);
        Serial.print("x");
        Serial.print((sizeinfo&0xffff));
        Serial.println(" pixels.");

        NUMPIXELS = (sizeinfo>>16)*(sizeinfo &0xffff);
        framebuffersize = NUMPIXELS*sizeof(unsigned int);

        ptr = (unsigned char*)malloc( framebuffersize * (hasdoublebuffer ? 3:2) );
        // Align.
        ptr += framebuffersize;
        ptr = (unsigned char*)((unsigned)ptr & ~(framebuffersize-1));

        memset(ptr,0x20,framebuffersize);

        screenBase = (volatile unsigned int*)ptr;
        REG(3) = (unsigned)ptr;
        /*
        if (hasdoublebuffer) {
            screen = screenBase + NUMPIXELS;
        } else {
            screen = screenBase;
            } */
        screen = (unsigned int*)malloc(framebuffersize);

        printf("Panel at slot %d, instance %d, base register 0x%08x\r\n",
               getSlot(), getInstance(), getBaseRegister());
#if 0
#ifdef INVERTXY
        Adafruit_GFX_32::begin(sizeinfo & 0xffff,sizeinfo>>16);
#else
        Adafruit_GFX_32::begin(sizeinfo>>16, sizeinfo & 0xffff);
#endif
#else
        Adafruit_GFX_32::begin(96,32);
#endif

        REG(2) = 0; // OE.
        //while (1);
    }
    void setOE(int i) {
        REG(2) = i&1;
    }
#ifdef INVERTXY
#else
    virtual void drawPixel(int x, int y, unsigned int v)
    {
        if (x<WIDTH && y<HEIGHT)
            screen[x+(y*WIDTH)]=v;
    }
#endif


#ifdef INVERTXY
#else
    virtual unsigned readPixel(int x, int y)
    {
        if (x<WIDTH && y<HEIGHT)
            return screen[x+(y*WIDTH)];
        return 0;
    }
#endif

    void clear(unsigned int color=0)
    {
        int index;
        for (index=0;index<NUMPIXELS;index++) {
            screen[index] = color;
        }
    }
#if 1
    virtual void fillRect(int x, int y, int w, int h,
                          unsigned int color) {
        int z;
        while (h--) {
            for (z=0;z<w;z++) {
                drawPixel(x+z,y,color);
            }
            y++;
        }
    }
#endif

    void test()
    {
    }

    bool applyEnabled;

    void setApplyEnabled(bool e) {
        applyEnabled=e;
    }

    void apply96 () {
        unsigned *fbptr = &screen[0];
        volatile unsigned *dest = (unsigned*)screenBase;
        int y,x;
        for (y=0;y<HEIGHT;y++) {
            dest += (128-96);
            for (x=0;x<96;x++) {
                *dest++ = *fbptr++;
            }
        }
    }

    void apply96_rev () {
        unsigned *fbptr = &screen[0];
        int y,x;
        for (y=0;y<HEIGHT;y++) {
            volatile unsigned *dest = (unsigned*)&screenBase[128*(31-y)];
            dest += (128-96);
            for (x=0;x<96;x++) {
                *dest++ = *fbptr++;
            }
        }
    }

    void apply96_rotate()
    {
        unsigned *fbptr = &screen[0];

        volatile unsigned *dest = (unsigned*)screenBase;
        dest+=32;
        volatile unsigned *sdest = dest;
        int y,x;
        for (y=0;y<96;y++) {
            //dest += (128-96);
            for (x=0;x<32;x++) {
                *dest = *fbptr++;
                dest+=128;
            }
            sdest++;
            dest = sdest;
        }

    }
    bool reverse;

    void setReverse(bool r)
    {
        reverse = r;
    }


    void apply()
    {
#if 0
        memcpy( (void*)screenBase, (void*)screen, framebuffersize);
#else
        if (reverse)
            apply96_rev();
        else
            apply96();
#endif
    }
    void shiftLeft(unsigned int color)
    {
        int x,y;
        for (y=0;y<HEIGHT;y++) {
            for (x=0;x<WIDTH-1;x++) {
                screen[x+(y*WIDTH)] = screen[1+x+(y*WIDTH)];
            }
            screen[(y*WIDTH) + (WIDTH-1)] = color;
        }
        apply();
    }
    void fadeLeft(unsigned int color, int len)
    {
        int i;
        for (i=0;i<len;i++) {
            shiftLeft(color);
            delay(20);
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


#ifdef INVERTXY
#else
    inline void setPixel(int x, int y, int r, int g, int b)
    {
        unsigned char c0 = linearTable[r&0xff];
        unsigned char c1 = linearTable[g&0xff];
        unsigned char c2 = linearTable[b&0xff];
        unsigned int v = c2 + (c1<<8) + (c0<<16);
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
#if 1
            unsigned int c0 = linearTable[v&0xff];
            unsigned int c1 = linearTable[(v>>8)&0xff];
            unsigned int c2 = linearTable[(v>>16)&0xff];
            v = c0 + (c1<<8) + (c2<<16);
#endif
            screen[i] = v;
            //screen[i] = 0xff0000;
            //screen[i+(32*32)] = v;
        }
    }

    void setImageAlpha(const unsigned int *base)
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

    void setImageAlphaRaw(const unsigned int *base)
    {
        int i,z;
        for (i=0;i<NUMPIXELS; i++) {
            // Load and parse each of the RGB.
            unsigned char *cptr = (unsigned char *)&base[i];
            unsigned char *sptr = (unsigned char *)&screen[i];
            unsigned alpha = cptr[0];
            alpha+=1;
            alpha<<=8;

            for (z=1;z<4;z++) {
                int src = cptr[z];
                int dst = sptr[z];
                dst = dst + (alpha*(src-dst))>>8;
                sptr[z] = dst;
                /*
                 d = sA*(1-p)+dA*p;
                 d = sA + p*dA - p*sA;
                 d = sA + p(dA-sA)
                    1 => d = sA+dA-sA == dA
                    0 => d = sA
                    0.5 => d = sA + 0.5dA - 0.5sA =
                */
            }
        }
    }
#endif
    volatile unsigned int *getScreen() {
        return screen;
    }
    inline void fastPrint(const char *c) {
        while (*c) {
            fastDrawChar(cursor_x, cursor_y, (unsigned char)*c, textcolor, textbgcolor, textsize);
            cursor_x += textsize*6;
            c++;
        };
    }

    // Draw a character
    void fastDrawChar(int x, int y, unsigned char c,
			    unsigned int color, unsigned int bg, uint8_t size) {

        for (int8_t i=0; i<6; i++ ) {
            uint8_t line;
            if (i == 5)
                line = 0x0;
            else
                line = pgm_read_byte(font+(c*5)+i);

            for (int8_t j = 0; j<7; j++) {
                if (line & 0x1) {
                    if (size == 1) // default size
                        drawPixel(x+i, y+j, color);
                    else {  // big size
                        fillRect(x+(i*size), y+(j*size), size, size, color);
                    }
                } else if (bg != color) {
                    if (size == 1) // default size
                        drawPixel(x+i, y+j, bg);
                    else {  // big size
                        fillRect(x+i*size, y+j*size, size, size, bg);
                    }
                }
                line >>= 1;
            }
        }
    }

    void prettyPrint(const char *str);

    volatile unsigned int *screenBase;
    unsigned int *screen;
};

#endif
