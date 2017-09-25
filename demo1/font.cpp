#include "font.h"
#include <Adafruit_GFX.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#define DEBUGFONT(x...) /* os_printf(x) */
#define DEBUG(x...) /* os_printf(x) */

extern unsigned char __tomthumb_bitmap__[];


static font_t *fonts = NULL; 
void font_init()
{
    font_t *tom_thumb_font = (font_t*)malloc(sizeof(font_t));
    tom_thumb_font->hdr.w=4;
    tom_thumb_font->hdr.h=6;
    tom_thumb_font->hdr.start=32;
    tom_thumb_font->hdr.end=126;
    tom_thumb_font->bitmap = __tomthumb_bitmap__;
    tom_thumb_font->ref=1;
    tom_thumb_font->next=NULL;
    strcpy(tom_thumb_font->name, "thumb");
    fonts = tom_thumb_font;
}

#ifndef ALIGN
#define ALIGN(x, alignment) (((x)+(alignment-1)) & ~(alignment-1))
#endif
#if 1

const font_t *  font_find(const char *name)
{
    font_t *font;
    char fname[64];

    int file;
    for (font = fonts; font; font=font->next) {
        if (strcmp(name,font->name)==0)
            return font;
    }
    sprintf(fname,"smallfs:/%s",name);

    file = open(fname, O_RDONLY);

    if (file>=0) {
        // Found. Allocate and link
        font = (font_t*)malloc(sizeof(font_t));

        if (NULL!=font) {
            read(file, &font->hdr, sizeof(font->hdr));
            DEBUGFONT("Loading font '%s', %dx%d, start %d end %d\n",
                      name,
                      (int)font->hdr.w,
                      (int)font->hdr.h,
                      (int)font->hdr.start,
                      (int)font->hdr.end);
            unsigned bpp = ALIGN((font->hdr.w-1),8) / 8;
            bpp *= ((font->hdr.end - font->hdr.start)+1);
            bpp *= font->hdr.h;
            strncpy(font->name,name,sizeof(font->name));
            font->bitmap = (uint8_t*)malloc(bpp);
            if (font->bitmap!=NULL) {
                read(file, font->bitmap, bpp);
                // Link it
                font->next = fonts;
                fonts = font;
            } else {
                free(font);
                font = NULL;
            }
        } else {
            free(font);
            font = NULL;
        }
        close(file);
    }

    if (NULL==font) {
        printf("Cannot find or alloc font '%s', reverting to 'thumb'\n", name);
        return font_find("thumb");
    }
    return font;
}
#endif

static int textComputeLength(const char *str, const textrendersettings_t *s, int *width, int *height);
static int getNumberOfPrintableChars(const char *str, const textrendersettings_t *settings, int*offset);

static int unpackHexByte(const char *str, uint8_t *dest);

static void drawChar16(Adafruit_GFX_32 *gfx, const font_t *font, int x, int y, unsigned char c,
                       color_t color, color_t bg)
{
    const uint8_t *cptr;
    //printf("Draw 16 %d %d \n", font->w, font->h);
    uint8_t hc = font->hdr.h;
    uint16_t mask = 0x8000;//(1<<(font->w-1));

    if ( (c<font->hdr.start) || (c>font->hdr.end)) {
        c = font->hdr.start;
    }
    cptr = (uint8_t*) ( &font->bitmap[(c - font->hdr.start)*(font->hdr.h)*2] );
    //printf("Start char %d, index is %d\n", c,  (c - font->start)*(font->h)*2 );
    // Draw.
    do {
        uint16_t line = (uint16_t)(*cptr++)<<8;
        line += (uint16_t)*cptr++;
        //printf("Line: 0x%04x mask 0x%04x\n",line,mask);
        uint8_t wc = font->hdr.w;
        int sx=x;
        do {
            int pixel = line & mask;
            line <<=1;
            if (pixel) {
                gfx->drawPixel(x, y, color);
            } else if (bg != color) {
                gfx->drawPixel(x, y, bg);
            }
            x++;
        } while (--wc);
        x=sx;
        y++;
    } while (--hc);
}


void drawChar(Adafruit_GFX_32 *gfx, const font_t *font, int x, int y, unsigned char c,
              color_t color, color_t bg, int direction)
{
    const uint8_t *cptr;
    int flip = direction & T_FLIP;
    int rotate = direction & T_ROTATE;

    if ((NULL==font) || (NULL==gfx)) {
        DEBUG("No font or no gfx!\n");
        return;
    }

    if (font->hdr.w > 8) {
        drawChar16(gfx, font, x, y, c, color, bg);
        return;
    }
    uint8_t hc = font->hdr.h;

    if ( (c<font->hdr.start) || (c>font->hdr.end)) {
        c = font->hdr.start;
    }
    cptr = &font->bitmap[(c - font->hdr.start)*(font->hdr.h)];

    // Draw.
    if (!rotate) {

        do {
            uint8_t line = *cptr++;
            uint8_t wc = font->hdr.w;
            int sx=x;
            do {
                int pixel = line & 0x80;
                line <<=1;
                if (pixel) {
                    gfx->drawPixel(x, y, color);
                } else if (bg != color) {
                    gfx->drawPixel(x, y, bg);
                }
                x++;
            } while (--wc);
            x=sx;
            y++;
        } while (--hc);
    } else {
        do {
            uint8_t line = *cptr++;
            uint8_t wc = font->hdr.w;
            int sy=y;
            do {
                int pixel;

                if (flip) {
                    pixel = (line & (0x1<<(8-font->hdr.w)));
                    line >>=1;
                } else {
                    pixel = line & 0x80;
                    line <<=1;
                }

                if (pixel) {
                    gfx->drawPixel(x, y, color);
                } else if (bg != color) {
                    gfx->drawPixel(x, y, bg);
                }
                y++;
            } while (--wc);
            y=sy;
            x++;
        } while (--hc);
    }

}

#if 1
void drawText(Adafruit_GFX_32  *gfx, const textrendersettings_t *s, int x, int y, const char *str, color_t color, color_t bg)
{
    int i,j;
    int sx = x;
    int offset;
    DEBUG("Draw text bg %d fg %d, maxwidth %d, wrap %d\n", (int)bg, (int)color, (int)s->w, (int)s->wrap);
    do {
        i = getNumberOfPrintableChars(str, s, &offset);
        if (i<0) {
            return;
        }
        x = sx;
#if 1
        if (s->w >0) {

            if (s->align == ALIGN_RIGHT ) {
                // Check excess

                int used = (s->font->hdr.w * i);
                //printf("Align right: used %d, avail %d\n", used, gfx->width);
                used = gfx->width() - used;
                //if (used<0)
                //    used=0;
                x+=used;
                //printf("Offset now %d fixup %d\n",x,used );
            }
        }
#endif
        DEBUG("Printing %d chars at %d %d\n", i, x, y);
        for (j=0;j<i;j++) {

            /*if (parseUnprintable(&str,&color,&bg)<0)
                return;
                DEBUG("Draw char '%c' at pos %d %d color 0x%02x\n", *str, x, y, color);
                */
            drawChar(gfx, s->font,x,y,*str,color,bg,s->direction);

            if (s->direction & T_ROTATE) { // Rotate
                y += s->font->hdr.w;
            } else {
                x += s->font->hdr.w;
            }
            str++;
        }
        if (s->direction & T_ROTATE) { // rotate
            x += s->font->hdr.h+1;
        } else {
            y += s->font->hdr.h+1;
        }

    } while (*str);
}

static int getNumberOfPrintableChars(const char *str,
                                     const textrendersettings_t *settings, int *offset)
{
    int skip;
    int count = 0;
    const char *lastspace = NULL;
    int maxwidth = settings->w;
    /* Skip spaces first? */
    *offset = 0;
    DEBUG("Maxw %d\n", maxwidth);
    if (*str==0)
        return 0;

    do {
        const char *save = str;
        skip = 0;//skipUnprintable(&str);
        if (skip<0) {
            return -1;
        }
        *offset += (str-save);

        if (maxwidth>=0) {
            if (maxwidth < settings->font->hdr.w) {

                if (settings->wrap && lastspace) {
                    //printf("lastspace\n");
                    lastspace++;
                    //printf("will restart at '%s'\n",lastspace);
                    count-=(str-lastspace);
                }
                return count;
            }
            if (isspace(*str))
                lastspace = str;
        }
        count++;
        str++;
        if (maxwidth>0) {
            maxwidth-=settings->font->hdr.w;
        }
    } while (*str);
    *offset+=count;
    return count;
}


static int textComputeLength(const char *str, const textrendersettings_t *s, int *width, int *height)
{
    int i;
    int maxw = 0;
    int maxh = 0;
    int offset;
    DEBUG("Computing length");
    do {
        DEBUG("Str: '%s' start 0x%02x\n", str, str[0]);
        i = getNumberOfPrintableChars(str, s, &offset);
        DEBUG("Printable chars: %d, offset %d\n", i, offset);
        if (i<0) {
            return -1;
        }
        maxh++;
        str+=offset;
        DEBUG("Str now: '%s'\n", str);
        if (maxw<i)
            maxw=i;
    } while (*str);
    *width = (maxw * s->font->hdr.w);// + (maxw-1);  // Spacing between chars
    *height = (maxh * s->font->hdr.h) + (maxh-1); // Spacing between chars
    return 0;
}
#endif

